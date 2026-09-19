#include "presentation/ui/AttractorPreviewWindow.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

void throwIfFailed(const VkResult result, const char* action) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(action) + " (VkResult=" + std::to_string(result) + ")");
    }
}

}  // namespace

AttractorPreviewWindow::~AttractorPreviewWindow() {
    cleanup();
}

void AttractorPreviewWindow::initialize(CreateInfo createInfo) {
    createInfo_ = std::move(createInfo);
    initialized_ = true;
}

void AttractorPreviewWindow::cleanup() {
    close();
    initialized_ = false;
}

void AttractorPreviewWindow::open() {
    if (window_ != nullptr) {
        return;
    }
    if (!initialized_) {
        throw std::runtime_error("Attractor preview window is not initialized.");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(
        createInfo_.width,
        createInfo_.height,
        createInfo_.title.c_str(),
        nullptr,
        nullptr);
    if (window_ == nullptr) {
        throw std::runtime_error("Failed to create attractor preview window.");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);

    try {
        createSurface();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createPipeline();
        createFramebuffers();
        createCommandResources();
        createSyncObjects();
    } catch (...) {
        close();
        throw;
    }
}

void AttractorPreviewWindow::close() {
    if (createInfo_.device != VK_NULL_HANDLE && window_ != nullptr) {
        vkDeviceWaitIdle(createInfo_.device);
    }

    cleanupSwapChain();

    if (inFlightFence_ != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
        vkDestroyFence(createInfo_.device, inFlightFence_, nullptr);
        inFlightFence_ = VK_NULL_HANDLE;
    }
    if (imageAvailableSemaphore_ != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
        vkDestroySemaphore(createInfo_.device, imageAvailableSemaphore_, nullptr);
        imageAvailableSemaphore_ = VK_NULL_HANDLE;
    }
    if (renderFinishedSemaphore_ != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
        vkDestroySemaphore(createInfo_.device, renderFinishedSemaphore_, nullptr);
        renderFinishedSemaphore_ = VK_NULL_HANDLE;
    }
    if (commandPool_ != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
        vkDestroyCommandPool(createInfo_.device, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }
    commandBuffer_ = VK_NULL_HANDLE;

    if (surface_ != VK_NULL_HANDLE && createInfo_.instance != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(createInfo_.instance, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    presentFamilyIndex_ = 0;
    presentQueue_ = VK_NULL_HANDLE;
    framebufferResized_ = false;
}

bool AttractorPreviewWindow::drawFrame(const DrawData& drawData) {
    if (window_ == nullptr || swapChain_ == VK_NULL_HANDLE) {
        return false;
    }

    throwIfFailed(
        vkWaitForFences(createInfo_.device, 1, &inFlightFence_, VK_TRUE, UINT64_MAX),
        "Failed to wait for attractor preview fence");

    uint32_t imageIndex = 0;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        createInfo_.device,
        swapChain_,
        UINT64_MAX,
        imageAvailableSemaphore_,
        VK_NULL_HANDLE,
        &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return true;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire attractor preview swapchain image.");
    }

    throwIfFailed(vkResetFences(createInfo_.device, 1, &inFlightFence_), "Failed to reset attractor preview fence");
    throwIfFailed(
        vkResetCommandBuffer(commandBuffer_, 0),
        "Failed to reset attractor preview command buffer");
    recordCommandBuffer(drawData, imageIndex);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore_};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore_};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer_;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (createInfo_.queueSubmitMutex != nullptr) {
        std::lock_guard<std::mutex> lock(*createInfo_.queueSubmitMutex);
        throwIfFailed(
            vkQueueSubmit(createInfo_.graphicsQueue, 1, &submitInfo, inFlightFence_),
            "Failed to submit attractor preview command buffer");
    } else {
        throwIfFailed(
            vkQueueSubmit(createInfo_.graphicsQueue, 1, &submitInfo, inFlightFence_),
            "Failed to submit attractor preview command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain_;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = VK_SUCCESS;
    if (createInfo_.queueSubmitMutex != nullptr) {
        std::lock_guard<std::mutex> lock(*createInfo_.queueSubmitMutex);
        presentResult = vkQueuePresentKHR(presentQueue_, &presentInfo);
    } else {
        presentResult = vkQueuePresentKHR(presentQueue_, &presentInfo);
    }

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR ||
        framebufferResized_) {
        framebufferResized_ = false;
        recreateSwapChain();
        return true;
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present attractor preview swapchain image.");
    }

    return false;
}

void AttractorPreviewWindow::setTitle(const std::string& title) {
    if (window_ != nullptr) {
        glfwSetWindowTitle(window_, title.c_str());
    }
}

bool AttractorPreviewWindow::isOpen() const {
    return window_ != nullptr;
}

bool AttractorPreviewWindow::shouldClose() const {
    return window_ != nullptr && glfwWindowShouldClose(window_) == GLFW_TRUE;
}

VkExtent2D AttractorPreviewWindow::extent() const {
    return swapChainExtent_;
}

void AttractorPreviewWindow::createSurface() {
    throwIfFailed(
        glfwCreateWindowSurface(createInfo_.instance, window_, nullptr, &surface_),
        "Failed to create attractor preview surface");

    const std::array<uint32_t, 2> candidateFamilies{{
        createInfo_.queueFamilyIndices.presentFamily.value(),
        createInfo_.queueFamilyIndices.graphicsFamily.value()
    }};
    for (const uint32_t familyIndex : candidateFamilies) {
        VkBool32 presentSupport = VK_FALSE;
        throwIfFailed(
            vkGetPhysicalDeviceSurfaceSupportKHR(createInfo_.physicalDevice, familyIndex, surface_, &presentSupport),
            "Failed to query attractor preview present support");
        if (presentSupport == VK_TRUE) {
            presentFamilyIndex_ = familyIndex;
            presentQueue_ =
                familyIndex == createInfo_.queueFamilyIndices.graphicsFamily.value()
                    ? createInfo_.graphicsQueue
                    : createInfo_.presentQueue;
            return;
        }
    }

    throw std::runtime_error("Selected GPU queue families cannot present the attractor preview surface.");
}

void AttractorPreviewWindow::createSwapChain() {
    const SwapChainSupportDetails swapChainSupport = querySwapChainSupport();
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
        throw std::runtime_error("Attractor preview surface does not support a Vulkan swapchain.");
    }

    const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D selectedExtent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = selectedExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t queueFamilyIndices[] = {
        createInfo_.queueFamilyIndices.graphicsFamily.value(),
        presentFamilyIndex_
    };
    if (createInfo_.queueFamilyIndices.graphicsFamily.value() != presentFamilyIndex_) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    throwIfFailed(
        vkCreateSwapchainKHR(createInfo_.device, &createInfo, nullptr, &swapChain_),
        "Failed to create attractor preview swapchain");

    throwIfFailed(
        vkGetSwapchainImagesKHR(createInfo_.device, swapChain_, &imageCount, nullptr),
        "Failed to query attractor preview swapchain images");
    swapChainImages_.resize(imageCount);
    throwIfFailed(
        vkGetSwapchainImagesKHR(createInfo_.device, swapChain_, &imageCount, swapChainImages_.data()),
        "Failed to get attractor preview swapchain images");

    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_ = selectedExtent;
    framebufferResized_ = false;
}

void AttractorPreviewWindow::createImageViews() {
    swapChainImageViews_.resize(swapChainImages_.size());
    for (std::size_t index = 0; index < swapChainImages_.size(); ++index) {
        swapChainImageViews_[index] = createImageView(swapChainImages_[index], swapChainImageFormat_);
    }
}

void AttractorPreviewWindow::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    throwIfFailed(
        vkCreateRenderPass(createInfo_.device, &renderPassInfo, nullptr, &renderPass_),
        "Failed to create attractor preview render pass");
}

void AttractorPreviewWindow::createPipeline() {
    const auto rectVertexCode = readBinaryFile(createInfo_.rectVertexShaderPath);
    const auto rectFragmentCode = readBinaryFile(createInfo_.rectFragmentShaderPath);
    const VkShaderModule rectVertexModule = createShaderModule(rectVertexCode);
    const VkShaderModule rectFragmentModule = createShaderModule(rectFragmentCode);

    VkPipelineShaderStageCreateInfo vertexStage{};
    vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStage.module = rectVertexModule;
    vertexStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStage{};
    fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStage.module = rectFragmentModule;
    fragmentStage.pName = "main";

    const VkPipelineShaderStageCreateInfo shaderStages[] = {vertexStage, fragmentStage};
    const auto solidBinding = Vertex::bindingDescription();
    const auto solidAttributes = Vertex::attributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &solidBinding;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(solidAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = solidAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent_.width);
    viewport.height = static_cast<float>(swapChainExtent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent_;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = createInfo_.solidPipelineLayout;
    pipelineInfo.renderPass = renderPass_;
    pipelineInfo.subpass = 0;

    const VkResult result = vkCreateGraphicsPipelines(
        createInfo_.device,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &solidPipeline_);
    vkDestroyShaderModule(createInfo_.device, rectFragmentModule, nullptr);
    vkDestroyShaderModule(createInfo_.device, rectVertexModule, nullptr);
    throwIfFailed(result, "Failed to create attractor preview pipeline");
}

void AttractorPreviewWindow::createFramebuffers() {
    framebuffers_.resize(swapChainImageViews_.size());
    for (std::size_t index = 0; index < swapChainImageViews_.size(); ++index) {
        VkImageView attachments[] = {swapChainImageViews_[index]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent_.width;
        framebufferInfo.height = swapChainExtent_.height;
        framebufferInfo.layers = 1;

        throwIfFailed(
            vkCreateFramebuffer(createInfo_.device, &framebufferInfo, nullptr, &framebuffers_[index]),
            "Failed to create attractor preview framebuffer");
    }
}

void AttractorPreviewWindow::createCommandResources() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = createInfo_.queueFamilyIndices.graphicsFamily.value();
    throwIfFailed(
        vkCreateCommandPool(createInfo_.device, &poolInfo, nullptr, &commandPool_),
        "Failed to create attractor preview command pool");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    throwIfFailed(
        vkAllocateCommandBuffers(createInfo_.device, &allocInfo, &commandBuffer_),
        "Failed to allocate attractor preview command buffer");
}

void AttractorPreviewWindow::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    throwIfFailed(
        vkCreateSemaphore(createInfo_.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore_),
        "Failed to create attractor preview image-available semaphore");
    throwIfFailed(
        vkCreateSemaphore(createInfo_.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore_),
        "Failed to create attractor preview render-finished semaphore");
    throwIfFailed(
        vkCreateFence(createInfo_.device, &fenceInfo, nullptr, &inFlightFence_),
        "Failed to create attractor preview in-flight fence");
}

void AttractorPreviewWindow::recreateSwapChain() {
    if (window_ == nullptr) {
        return;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
        glfwWaitEvents();
        if (window_ == nullptr) {
            return;
        }
        glfwGetFramebufferSize(window_, &width, &height);
    }

    vkDeviceWaitIdle(createInfo_.device);

    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createPipeline();
    createFramebuffers();
}

void AttractorPreviewWindow::cleanupSwapChain() {
    for (VkFramebuffer framebuffer : framebuffers_) {
        if (framebuffer != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(createInfo_.device, framebuffer, nullptr);
        }
    }
    framebuffers_.clear();

    if (solidPipeline_ != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
        vkDestroyPipeline(createInfo_.device, solidPipeline_, nullptr);
        solidPipeline_ = VK_NULL_HANDLE;
    }
    if (renderPass_ != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
        vkDestroyRenderPass(createInfo_.device, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }

    for (VkImageView imageView : swapChainImageViews_) {
        if (imageView != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
            vkDestroyImageView(createInfo_.device, imageView, nullptr);
        }
    }
    swapChainImageViews_.clear();

    if (swapChain_ != VK_NULL_HANDLE && createInfo_.device != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(createInfo_.device, swapChain_, nullptr);
        swapChain_ = VK_NULL_HANDLE;
    }

    swapChainImages_.clear();
    swapChainImageFormat_ = VK_FORMAT_UNDEFINED;
    swapChainExtent_ = {};
}

void AttractorPreviewWindow::recordCommandBuffer(const DrawData& drawData, const uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    throwIfFailed(vkBeginCommandBuffer(commandBuffer_, &beginInfo), "Failed to begin attractor preview command buffer");

    VkClearValue clearColor{};
    clearColor.color = {{
        createInfo_.clearColor[0],
        createInfo_.clearColor[1],
        createInfo_.clearColor[2],
        createInfo_.clearColor[3]
    }};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = framebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent_;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(commandBuffer_, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (drawData.vertexCount > 0U && drawData.vertexBuffer != VK_NULL_HANDLE) {
        const VkDeviceSize zeroOffset = 0;
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, solidPipeline_);
        vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &drawData.vertexBuffer, &zeroOffset);

        const auto drawRange = [&](const SolidDrawRange& range, const std::array<float, 4>& color) {
            RectPushConstants push{};
            std::memcpy(push.color, color.data(), sizeof(push.color));
            vkCmdPushConstants(
                commandBuffer_,
                createInfo_.solidPipelineLayout,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(RectPushConstants),
                &push);
            vkCmdDraw(commandBuffer_, range.vertexCount, 1, range.firstVertex, 0);
        };

        if (drawData.ranges.edges.vertexCount > 0U) {
            drawRange(drawData.ranges.edges, drawData.edgeColor);
        }
        if (drawData.ranges.nodes.vertexCount > 0U) {
            drawRange(drawData.ranges.nodes, drawData.nodeColor);
        }
        if (drawData.ranges.cycles.vertexCount > 0U) {
            drawRange(drawData.ranges.cycles, drawData.cycleColor);
        }
        if (drawData.ranges.legendPanel.vertexCount > 0U) {
            drawRange(drawData.ranges.legendPanel, drawData.panelColor);
        }
        if (drawData.ranges.legendEdgeSwatch.vertexCount > 0U) {
            drawRange(drawData.ranges.legendEdgeSwatch, drawData.edgeColor);
        }
        if (drawData.ranges.legendNodeSwatch.vertexCount > 0U) {
            drawRange(drawData.ranges.legendNodeSwatch, drawData.nodeColor);
        }
        if (drawData.ranges.legendCycleSwatch.vertexCount > 0U) {
            drawRange(drawData.ranges.legendCycleSwatch, drawData.cycleColor);
        }
        if (drawData.ranges.legendText.vertexCount > 0U) {
            drawRange(drawData.ranges.legendText, drawData.textColor);
        }
    }

    vkCmdEndRenderPass(commandBuffer_);
    throwIfFailed(vkEndCommandBuffer(commandBuffer_), "Failed to record attractor preview command buffer");
}

SwapChainSupportDetails AttractorPreviewWindow::querySwapChainSupport() const {
    SwapChainSupportDetails details{};

    throwIfFailed(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(createInfo_.physicalDevice, surface_, &details.capabilities),
        "Failed to query attractor preview surface capabilities");

    uint32_t formatCount = 0;
    throwIfFailed(
        vkGetPhysicalDeviceSurfaceFormatsKHR(createInfo_.physicalDevice, surface_, &formatCount, nullptr),
        "Failed to query attractor preview surface formats");
    if (formatCount > 0) {
        details.formats.resize(formatCount);
        throwIfFailed(
            vkGetPhysicalDeviceSurfaceFormatsKHR(createInfo_.physicalDevice, surface_, &formatCount, details.formats.data()),
            "Failed to query attractor preview surface formats");
    }

    uint32_t presentModeCount = 0;
    throwIfFailed(
        vkGetPhysicalDeviceSurfacePresentModesKHR(createInfo_.physicalDevice, surface_, &presentModeCount, nullptr),
        "Failed to query attractor preview surface present modes");
    if (presentModeCount > 0) {
        details.presentModes.resize(presentModeCount);
        throwIfFailed(
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                createInfo_.physicalDevice,
                surface_,
                &presentModeCount,
                details.presentModes.data()),
            "Failed to query attractor preview surface present modes");
    }

    return details;
}

VkSurfaceFormatKHR AttractorPreviewWindow::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats.front();
}

VkPresentModeKHR AttractorPreviewWindow::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) const {
    for (const VkPresentModeKHR availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    for (const VkPresentModeKHR availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            return availablePresentMode;
        }
    }

    for (const VkPresentModeKHR availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D AttractorPreviewWindow::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(
        actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return actualExtent;
}

VkShaderModule AttractorPreviewWindow::createShaderModule(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    throwIfFailed(
        vkCreateShaderModule(createInfo_.device, &createInfo, nullptr, &shaderModule),
        "Failed to create attractor preview shader module");
    return shaderModule;
}

VkImageView AttractorPreviewWindow::createImageView(const VkImage image, const VkFormat format) const {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = VK_NULL_HANDLE;
    throwIfFailed(
        vkCreateImageView(createInfo_.device, &viewInfo, nullptr, &imageView),
        "Failed to create attractor preview image view");
    return imageView;
}

VkVertexInputBindingDescription AttractorPreviewWindow::Vertex::bindingDescription() {
    VkVertexInputBindingDescription description{};
    description.binding = 0;
    description.stride = sizeof(Vertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return description;
}

std::array<VkVertexInputAttributeDescription, 1> AttractorPreviewWindow::Vertex::attributeDescriptions() {
    return {{
        VkVertexInputAttributeDescription{
            0,
            0,
            VK_FORMAT_R32G32_SFLOAT,
            static_cast<uint32_t>(offsetof(Vertex, position))
        }
    }};
}

void AttractorPreviewWindow::framebufferResizeCallback(GLFWwindow* window, int, int) {
    auto* previewWindow = reinterpret_cast<AttractorPreviewWindow*>(glfwGetWindowUserPointer(window));
    if (previewWindow != nullptr && previewWindow->window_ == window) {
        previewWindow->framebufferResized_ = true;
    }
}
