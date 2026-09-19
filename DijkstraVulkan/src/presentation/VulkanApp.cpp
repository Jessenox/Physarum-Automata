#include "presentation/VulkanApp.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {

constexpr float kLogicalWidth = 900.0f;
constexpr float kLogicalHeight = 700.0f;
constexpr float kCanvas = 500.0f;
constexpr std::array<float, 4> kClear{{0.025f, 0.030f, 0.040f, 1.0f}};
constexpr std::array<float, 4> kPanel{{0.090f, 0.105f, 0.130f, 1.0f}};
constexpr std::array<float, 4> kSidebar{{0.055f, 0.065f, 0.085f, 1.0f}};
constexpr std::array<float, 4> kButton{{0.145f, 0.175f, 0.220f, 1.0f}};
constexpr std::array<float, 4> kButtonSelected{{0.10f, 0.42f, 0.48f, 1.0f}};
constexpr std::array<float, 4> kText{{0.89f, 0.92f, 0.96f, 1.0f}};
constexpr std::array<float, 4> kMuted{{0.52f, 0.59f, 0.68f, 1.0f}};
constexpr std::array<float, 4> kCyan{{0.05f, 0.78f, 0.92f, 1.0f}};
constexpr std::array<float, 4> kGreen{{0.18f, 0.92f, 0.48f, 1.0f}};
constexpr std::array<float, 4> kPink{{0.98f, 0.25f, 0.65f, 1.0f}};
constexpr std::array<float, 4> kGold{{1.0f, 0.72f, 0.16f, 1.0f}};

float ndcX(const float x) {
    return (x / kLogicalWidth) * 2.0f - 1.0f;
}

float ndcY(const float y) {
    return (y / kLogicalHeight) * 2.0f - 1.0f;
}

void throwIfFailed(const VkResult result, const std::string_view action) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(action) + " (VkResult=" + std::to_string(result) + ")");
    }
}

std::vector<char> readBinary(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open shader: " + path.string());
    }
    const std::streamsize size = file.tellg();
    std::vector<char> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    if (size > 0 && !file.read(bytes.data(), size)) {
        throw std::runtime_error("Could not read shader: " + path.string());
    }
    return bytes;
}

std::array<uint8_t, 7> glyph(const char value) {
    switch (value) {
        case '0': return {{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}};
        case '1': return {{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}};
        case '2': return {{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}};
        case '3': return {{0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}};
        case '4': return {{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}};
        case '5': return {{0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}};
        case '6': return {{0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}};
        case '7': return {{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}};
        case '8': return {{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}};
        case '9': return {{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}};
        case 'A': return {{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
        case 'B': return {{0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}};
        case 'C': return {{0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}};
        case 'D': return {{0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}};
        case 'E': return {{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}};
        case 'F': return {{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}};
        case 'G': return {{0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}};
        case 'H': return {{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
        case 'I': return {{0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}};
        case 'J': return {{0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}};
        case 'K': return {{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}};
        case 'L': return {{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}};
        case 'M': return {{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}};
        case 'N': return {{0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}};
        case 'O': return {{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}};
        case 'P': return {{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}};
        case 'Q': return {{0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}};
        case 'R': return {{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}};
        case 'S': return {{0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}};
        case 'T': return {{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}};
        case 'U': return {{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}};
        case 'V': return {{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}};
        case 'W': return {{0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}};
        case 'X': return {{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}};
        case 'Y': return {{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}};
        case 'Z': return {{0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}};
        case ':': return {{0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00}};
        case '.': return {{0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}};
        case '/': return {{0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}};
        case '-': return {{0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}};
        default: return {{0, 0, 0, 0, 0, 0, 0}};
    }
}

}  // namespace

VulkanApp::VulkanApp(const GridSize size, const bool autoVerify)
    : requestedGridSize_(size), autoVerify_(autoVerify) {}

VulkanApp::~VulkanApp() {
    cleanup();
}

void VulkanApp::run() {
    initWindow();
    initVulkan();
    mainLoop();
}

void VulkanApp::initWindow() {
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("GLFW initialization failed.");
    }
    if (glfwVulkanSupported() != GLFW_TRUE) {
        throw std::runtime_error("GLFW did not find a Vulkan loader.");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, "DijkstraVulkan", nullptr, nullptr);
    if (window_ == nullptr) {
        throw std::runtime_error("Could not create the DijkstraVulkan window.");
    }
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}

void VulkanApp::initVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createDevice();
    initializeGridModel();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorLayouts();
    createPipelineLayouts();
    createGraphicsPipelines();
    createComputePipelines();
    createFramebuffers();
    createCommandResources();
    createBuffers();
    createDescriptors();
    createSyncObjects();
    uploadModel();
    if (autoVerify_) {
        running_ = true;
        stepsPerFrame_ = 64U;
        status_ = "VERIFICANDO GPU";
    }
    rebuildUi();
    refreshTitle();
    std::cout << "DijkstraVulkan GPU: " << gpuName_ << '\n'
              << "Grid: " << model_->size().width << 'x' << model_->size().height
              << " (" << model_->nodeCount() << " nodes)\n"
              << "Hardware buffer capacity: " << hardwareNodeCapacity_ << " nodes\n"
              << "Interactive safety capacity: " << maximumNodeCount_ << " nodes\n"
              << "Safe compute rate: " << safeStepsPerFrame() << " Dijkstra steps/frame\n"
              << "Compute memory: "
              << ((storageMemoryProperties_ & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0U
                      ? "DEVICE_LOCAL + HOST_VISIBLE"
                      : "HOST_VISIBLE")
              << '\n';
}

void VulkanApp::mainLoop() {
    constexpr auto kMinimumFrameTime = std::chrono::microseconds(16'667);
    auto nextFrame = std::chrono::steady_clock::now();
    while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
        glfwPollEvents();
        waitForFrame();
        consumeGpuResult();
        processInput();
        rebuildUi();
        refreshTitle();
        drawFrame();

        nextFrame += kMinimumFrameTime;
        const auto now = std::chrono::steady_clock::now();
        if (nextFrame > now) {
            std::this_thread::sleep_until(nextFrame);
        } else {
            nextFrame = now;
        }
    }
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
}

void VulkanApp::cleanup() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    cleanupSwapchain();
    if (resetPipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, resetPipeline_, nullptr);
    if (clearPipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, clearPipeline_, nullptr);
    if (findMinPipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, findMinPipeline_, nullptr);
    if (chooseMinPipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, chooseMinPipeline_, nullptr);
    if (relaxPipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, relaxPipeline_, nullptr);
    if (gridPipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, gridPipelineLayout_, nullptr);
    if (uiPipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, uiPipelineLayout_, nullptr);
    if (computePipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, computePipelineLayout_, nullptr);
    if (gridDescriptorLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device_, gridDescriptorLayout_, nullptr);
    if (computeDescriptorLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device_, computeDescriptorLayout_, nullptr);
    if (descriptorPool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

    destroyBuffer(controlBuffer_);
    destroyBuffer(visitedBuffer_);
    destroyBuffer(previousBuffer_);
    destroyBuffer(distancesBuffer_);
    destroyBuffer(cellsBuffer_);
    destroyBuffer(uiVertexBuffer_);
    destroyBuffer(gridVertexBuffer_);

    if (inFlightFence_ != VK_NULL_HANDLE) vkDestroyFence(device_, inFlightFence_, nullptr);
    if (renderFinished_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, renderFinished_, nullptr);
    if (imageAvailable_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, imageAvailable_, nullptr);
    if (commandPool_ != VK_NULL_HANDLE) vkDestroyCommandPool(device_, commandPool_, nullptr);
    if (device_ != VK_NULL_HANDLE) vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;

    if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    if (instance_ != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
    if (window_ != nullptr) glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();
}

void VulkanApp::cleanupSwapchain() {
    if (device_ == VK_NULL_HANDLE) return;
    for (const VkFramebuffer framebuffer : framebuffers_) vkDestroyFramebuffer(device_, framebuffer, nullptr);
    framebuffers_.clear();
    if (gridPipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, gridPipeline_, nullptr);
    if (uiPipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, uiPipeline_, nullptr);
    gridPipeline_ = VK_NULL_HANDLE;
    uiPipeline_ = VK_NULL_HANDLE;
    if (renderPass_ != VK_NULL_HANDLE) vkDestroyRenderPass(device_, renderPass_, nullptr);
    renderPass_ = VK_NULL_HANDLE;
    for (const VkImageView view : swapchainImageViews_) vkDestroyImageView(device_, view, nullptr);
    swapchainImageViews_.clear();
    if (swapchain_ != VK_NULL_HANDLE) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
    swapchainImages_.clear();
}

void VulkanApp::recreateSwapchain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window_, &width, &height);
    }
    vkDeviceWaitIdle(device_);
    cleanupSwapchain();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createGraphicsPipelines();
    createFramebuffers();
    framebufferResized_ = false;
}

void VulkanApp::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DijkstraVulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Physarum UI";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (extensions == nullptr) throw std::runtime_error("GLFW returned no Vulkan instance extensions.");

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    throwIfFailed(vkCreateInstance(&createInfo, nullptr, &instance_), "Could not create Vulkan instance");
}

void VulkanApp::createSurface() {
    throwIfFailed(glfwCreateWindowSurface(instance_, window_, nullptr, &surface_), "Could not create window surface");
}

void VulkanApp::pickPhysicalDevice() {
    uint32_t count = 0;
    throwIfFailed(vkEnumeratePhysicalDevices(instance_, &count, nullptr), "Could not enumerate GPUs");
    if (count == 0) throw std::runtime_error("No Vulkan GPU was found.");
    std::vector<VkPhysicalDevice> devices(count);
    throwIfFailed(vkEnumeratePhysicalDevices(instance_, &count, devices.data()), "Could not read GPU list");

    int bestScore = -1;
    for (const VkPhysicalDevice candidate : devices) {
        const QueueFamilies families = findQueueFamilies(candidate);
        if (!families.complete() || !deviceSupportsSwapchain(candidate)) continue;
        const SwapSupport swap = querySwapSupport(candidate);
        if (swap.formats.empty() || swap.presentModes.empty()) continue;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 100 : 10;
        if (score > bestScore) {
            bestScore = score;
            physicalDevice_ = candidate;
            queueFamilies_ = families;
            gpuName_ = properties.deviceName;
        }
    }
    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("No GPU supports graphics, compute, presentation, and swapchain.");
    }
    vkGetPhysicalDeviceProperties(physicalDevice_, &physicalDeviceProperties_);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &physicalDeviceMemoryProperties_);
}

void VulkanApp::createDevice() {
    const std::set<uint32_t> uniqueFamilies{
        queueFamilies_.graphicsCompute.value(),
        queueFamilies_.present.value()
    };
    constexpr float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for (const uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = family;
        info.queueCount = 1;
        info.pQueuePriorities = &priority;
        queueInfos.push_back(info);
    }
    const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = extensions;
    throwIfFailed(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_), "Could not create Vulkan device");
    vkGetDeviceQueue(device_, queueFamilies_.graphicsCompute.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, queueFamilies_.present.value(), 0, &presentQueue_);
}

void VulkanApp::initializeGridModel() {
    constexpr VkMemoryPropertyFlags requiredHostFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint64_t bestHeapSize = 0;
    bool bestIsDeviceLocal = false;
    bool memoryTypeFound = false;
    for (uint32_t index = 0; index < physicalDeviceMemoryProperties_.memoryTypeCount; ++index) {
        const VkMemoryType& type = physicalDeviceMemoryProperties_.memoryTypes[index];
        if ((type.propertyFlags & requiredHostFlags) != requiredHostFlags) continue;
        const bool deviceLocal = (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0U;
        const uint64_t heapSize = physicalDeviceMemoryProperties_.memoryHeaps[type.heapIndex].size;
        if (!memoryTypeFound || heapSize > bestHeapSize || (heapSize == bestHeapSize && deviceLocal && !bestIsDeviceLocal)) {
            memoryTypeFound = true;
            bestHeapSize = heapSize;
            bestIsDeviceLocal = deviceLocal;
            storageMemoryHeapIndex_ = type.heapIndex;
            storageMemoryProperties_ = requiredHostFlags |
                (deviceLocal ? static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) : 0U);
        }
    }
    if (!memoryTypeFound) {
        throw std::runtime_error("The selected GPU has no host-visible coherent memory for compute buffers.");
    }

    // Two uint32 arrays plus packed 8-bit weights and packed 2-bit visit states
    // consume about 9.25 bytes/node. Ten bytes keeps alignment headroom. The
    // hardware estimate uses at most 25% of the compatible heap so the desktop
    // and driver retain ample memory even before the interactive work cap.
    constexpr uint64_t bytesPerNodeBudget = 10U;
    const uint64_t heapBudgetNodes = (bestHeapSize * 25U / 100U) / bytesPerNodeBudget;
    const uint64_t storageRangeNodes = physicalDeviceProperties_.limits.maxStorageBufferRange / sizeof(uint32_t);
    const uint64_t dispatchNodes =
        static_cast<uint64_t>(physicalDeviceProperties_.limits.maxComputeWorkGroupCount[0]) * 256U;
    hardwareNodeCapacity_ = std::min({
        heapBudgetNodes,
        storageRangeNodes,
        dispatchNodes,
        static_cast<uint64_t>(kInvalidNode) - 1U
    });
    maximumNodeCount_ = std::min(hardwareNodeCapacity_, kDijkstraInteractiveNodeLimit);

    const uint64_t requestedNodes =
        static_cast<uint64_t>(requestedGridSize_.width) * requestedGridSize_.height;
    if (requestedNodes > maximumNodeCount_) {
        const uint64_t maximumSquareSide = static_cast<uint64_t>(std::sqrt(static_cast<long double>(maximumNodeCount_)));
        std::ostringstream message;
        message << "Requested grid " << requestedGridSize_.width << 'x' << requestedGridSize_.height
                << " has " << requestedNodes << " nodes, but the dense interactive backend is limited to "
                << maximumNodeCount_ << " nodes (about " << maximumSquareSide << 'x'
                << maximumSquareSide << " if square) to avoid exhausting memory or monopolizing the desktop GPU. "
                << "The raw hardware buffer capacity is " << hardwareNodeCapacity_ << " nodes.";
        throw std::runtime_error(message.str());
    }

    try {
        model_ = std::make_unique<GridModel>(requestedGridSize_);
    } catch (const std::bad_alloc&) {
        throw std::runtime_error("Host RAM could not allocate the requested grid model.");
    }
}

void VulkanApp::createSwapchain() {
    const SwapSupport support = querySwapSupport(physicalDevice_);
    const VkSurfaceFormatKHR format = chooseSurfaceFormat(support.formats);
    const VkPresentModeKHR presentMode = choosePresentMode(support.presentModes);
    const VkExtent2D extent = chooseExtent(support.capabilities);
    uint32_t imageCount = support.capabilities.minImageCount + 1U;
    if (support.capabilities.maxImageCount > 0U) imageCount = std::min(imageCount, support.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    const uint32_t familyIndices[] = {queueFamilies_.graphicsCompute.value(), queueFamilies_.present.value()};
    if (familyIndices[0] != familyIndices[1]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = familyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    throwIfFailed(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_), "Could not create swapchain");

    throwIfFailed(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr), "Could not count swapchain images");
    swapchainImages_.resize(imageCount);
    throwIfFailed(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data()), "Could not get swapchain images");
    swapchainFormat_ = format.format;
    swapchainExtent_ = extent;
}

void VulkanApp::createImageViews() {
    swapchainImageViews_.resize(swapchainImages_.size());
    for (std::size_t index = 0; index < swapchainImages_.size(); ++index) {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = swapchainImages_[index];
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = swapchainFormat_;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;
        throwIfFailed(vkCreateImageView(device_, &info, nullptr, &swapchainImageViews_[index]), "Could not create image view");
    }
}

void VulkanApp::createRenderPass() {
    VkAttachmentDescription color{};
    color.format = swapchainFormat_;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &color;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;
    throwIfFailed(vkCreateRenderPass(device_, &info, nullptr, &renderPass_), "Could not create render pass");
}

void VulkanApp::createDescriptorLayouts() {
    std::array<VkDescriptorSetLayoutBinding, 3> gridBindings{};
    for (uint32_t binding = 0; binding < gridBindings.size(); ++binding) {
        gridBindings[binding].binding = binding;
        gridBindings[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        gridBindings[binding].descriptorCount = 1;
        gridBindings[binding].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo gridInfo{};
    gridInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    gridInfo.bindingCount = static_cast<uint32_t>(gridBindings.size());
    gridInfo.pBindings = gridBindings.data();
    throwIfFailed(vkCreateDescriptorSetLayout(device_, &gridInfo, nullptr, &gridDescriptorLayout_), "Could not create grid descriptors");

    std::array<VkDescriptorSetLayoutBinding, 5> computeBindings{};
    for (uint32_t binding = 0; binding < computeBindings.size(); ++binding) {
        computeBindings[binding].binding = binding;
        computeBindings[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        computeBindings[binding].descriptorCount = 1;
        computeBindings[binding].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo computeInfo{};
    computeInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    computeInfo.bindingCount = static_cast<uint32_t>(computeBindings.size());
    computeInfo.pBindings = computeBindings.data();
    throwIfFailed(vkCreateDescriptorSetLayout(device_, &computeInfo, nullptr, &computeDescriptorLayout_), "Could not create compute descriptors");
}

void VulkanApp::createPipelineLayouts() {
    VkPipelineLayoutCreateInfo gridInfo{};
    gridInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    gridInfo.setLayoutCount = 1;
    gridInfo.pSetLayouts = &gridDescriptorLayout_;
    throwIfFailed(vkCreatePipelineLayout(device_, &gridInfo, nullptr, &gridPipelineLayout_), "Could not create grid pipeline layout");

    VkPushConstantRange uiPush{};
    uiPush.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    uiPush.size = sizeof(float) * 4U;
    VkPipelineLayoutCreateInfo uiInfo{};
    uiInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    uiInfo.pushConstantRangeCount = 1;
    uiInfo.pPushConstantRanges = &uiPush;
    throwIfFailed(vkCreatePipelineLayout(device_, &uiInfo, nullptr, &uiPipelineLayout_), "Could not create UI pipeline layout");

    VkPipelineLayoutCreateInfo computeInfo{};
    computeInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeInfo.setLayoutCount = 1;
    computeInfo.pSetLayouts = &computeDescriptorLayout_;
    throwIfFailed(vkCreatePipelineLayout(device_, &computeInfo, nullptr, &computePipelineLayout_), "Could not create compute pipeline layout");
}

void VulkanApp::createGraphicsPipelines() {
    const VkShaderModule gridVertex = createShaderModule(shaderPath("grid.vert.spv"));
    const VkShaderModule gridFragment = createShaderModule(shaderPath("grid.frag.spv"));
    const VkShaderModule uiVertex = createShaderModule(shaderPath("ui.vert.spv"));
    const VkShaderModule uiFragment = createShaderModule(shaderPath("ui.frag.spv"));

    const auto makePipeline = [&](const VkShaderModule vertexModule,
                                  const VkShaderModule fragmentModule,
                                  const uint32_t stride,
                                  const std::vector<VkVertexInputAttributeDescription>& attributes,
                                  const VkPipelineLayout layout,
                                  VkPipeline* pipeline) {
        VkPipelineShaderStageCreateInfo vertexStage{};
        vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexStage.module = vertexModule;
        vertexStage.pName = "main";
        VkPipelineShaderStageCreateInfo fragmentStage{};
        fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentStage.module = fragmentModule;
        fragmentStage.pName = "main";
        const VkPipelineShaderStageCreateInfo stages[] = {vertexStage, fragmentStage};

        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = stride;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInput.pVertexAttributeDescriptions = attributes.data();
        VkPipelineInputAssemblyStateCreateInfo assembly{};
        assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkViewport viewport{0.0f, 0.0f, static_cast<float>(swapchainExtent_.width), static_cast<float>(swapchainExtent_.height), 0.0f, 1.0f};
        VkRect2D scissor{{0, 0}, swapchainExtent_};
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
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo blending{};
        blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blending.attachmentCount = 1;
        blending.pAttachments = &blendAttachment;
        VkGraphicsPipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &vertexInput;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewportState;
        info.pRasterizationState = &rasterizer;
        info.pMultisampleState = &multisampling;
        info.pColorBlendState = &blending;
        info.layout = layout;
        info.renderPass = renderPass_;
        throwIfFailed(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &info, nullptr, pipeline), "Could not create graphics pipeline");
    };

    const std::vector<VkVertexInputAttributeDescription> gridAttributes{
        {0, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(TexturedVertex, position))},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(TexturedVertex, uv))},
    };
    const std::vector<VkVertexInputAttributeDescription> uiAttributes{
        {0, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(UiVertex, position))},
    };
    try {
        makePipeline(gridVertex, gridFragment, sizeof(TexturedVertex), gridAttributes, gridPipelineLayout_, &gridPipeline_);
        makePipeline(uiVertex, uiFragment, sizeof(UiVertex), uiAttributes, uiPipelineLayout_, &uiPipeline_);
    } catch (...) {
        vkDestroyShaderModule(device_, uiFragment, nullptr);
        vkDestroyShaderModule(device_, uiVertex, nullptr);
        vkDestroyShaderModule(device_, gridFragment, nullptr);
        vkDestroyShaderModule(device_, gridVertex, nullptr);
        throw;
    }
    vkDestroyShaderModule(device_, uiFragment, nullptr);
    vkDestroyShaderModule(device_, uiVertex, nullptr);
    vkDestroyShaderModule(device_, gridFragment, nullptr);
    vkDestroyShaderModule(device_, gridVertex, nullptr);
}

void VulkanApp::createComputePipelines() {
    resetPipeline_ = createComputePipeline("dijkstra_reset.comp.spv");
    clearPipeline_ = createComputePipeline("dijkstra_clear.comp.spv");
    findMinPipeline_ = createComputePipeline("dijkstra_find_min.comp.spv");
    chooseMinPipeline_ = createComputePipeline("dijkstra_choose_min.comp.spv");
    relaxPipeline_ = createComputePipeline("dijkstra_relax.comp.spv");
}

void VulkanApp::createFramebuffers() {
    framebuffers_.resize(swapchainImageViews_.size());
    for (std::size_t index = 0; index < swapchainImageViews_.size(); ++index) {
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderPass_;
        info.attachmentCount = 1;
        info.pAttachments = &swapchainImageViews_[index];
        info.width = swapchainExtent_.width;
        info.height = swapchainExtent_.height;
        info.layers = 1;
        throwIfFailed(vkCreateFramebuffer(device_, &info, nullptr, &framebuffers_[index]), "Could not create framebuffer");
    }
}

void VulkanApp::createCommandResources() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilies_.graphicsCompute.value();
    throwIfFailed(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_), "Could not create command pool");
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    throwIfFailed(vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer_), "Could not allocate command buffer");
}

void VulkanApp::createBuffers() {
    const std::array<TexturedVertex, 6> quad{{
        TexturedVertex{{ndcX(0.0f), ndcY(0.0f)}, {0.0f, 0.0f}},
        TexturedVertex{{ndcX(kCanvas), ndcY(0.0f)}, {1.0f, 0.0f}},
        TexturedVertex{{ndcX(kCanvas), ndcY(kCanvas)}, {1.0f, 1.0f}},
        TexturedVertex{{ndcX(0.0f), ndcY(0.0f)}, {0.0f, 0.0f}},
        TexturedVertex{{ndcX(kCanvas), ndcY(kCanvas)}, {1.0f, 1.0f}},
        TexturedVertex{{ndcX(0.0f), ndcY(kCanvas)}, {0.0f, 1.0f}},
    }};
    constexpr VkMemoryPropertyFlags hostMemory = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    createBuffer(sizeof(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, hostMemory, gridVertexBuffer_);
    std::memcpy(gridVertexBuffer_.mapped, quad.data(), sizeof(quad));
    createBuffer(sizeof(UiVertex) * kUiVertexCapacity, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, hostMemory, uiVertexBuffer_);

    const VkDeviceSize nodeDataSize = sizeof(uint32_t) * static_cast<VkDeviceSize>(model_->nodeCount());
    const VkDeviceSize packedCellSize = sizeof(uint32_t) * ((static_cast<VkDeviceSize>(model_->nodeCount()) + 3U) / 4U);
    const VkDeviceSize packedVisitedSize = sizeof(uint32_t) * ((static_cast<VkDeviceSize>(model_->nodeCount()) + 15U) / 16U);
    createBuffer(packedCellSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, storageMemoryProperties_, cellsBuffer_);
    createBuffer(nodeDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, storageMemoryProperties_, distancesBuffer_);
    createBuffer(nodeDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, storageMemoryProperties_, previousBuffer_);
    createBuffer(packedVisitedSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, storageMemoryProperties_, visitedBuffer_);
    createBuffer(sizeof(GpuControl), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, storageMemoryProperties_, controlBuffer_);
    distances_ = static_cast<uint32_t*>(distancesBuffer_.mapped);
    previous_ = static_cast<uint32_t*>(previousBuffer_.mapped);
    packedCells_ = static_cast<uint32_t*>(cellsBuffer_.mapped);
    packedVisited_ = static_cast<uint32_t*>(visitedBuffer_.mapped);
    control_ = static_cast<GpuControl*>(controlBuffer_.mapped);
}

void VulkanApp::createDescriptors() {
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 8};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 2;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    throwIfFailed(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_), "Could not create descriptor pool");
    const VkDescriptorSetLayout layouts[] = {gridDescriptorLayout_, computeDescriptorLayout_};
    VkDescriptorSet sets[2]{};
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 2;
    allocInfo.pSetLayouts = layouts;
    throwIfFailed(vkAllocateDescriptorSets(device_, &allocInfo, sets), "Could not allocate descriptor sets");
    gridDescriptorSet_ = sets[0];
    computeDescriptorSet_ = sets[1];

    const auto bufferInfo = [](const Buffer& buffer) {
        return VkDescriptorBufferInfo{buffer.handle, 0, buffer.size};
    };
    const std::array<VkDescriptorBufferInfo, 3> gridInfos{{
        bufferInfo(cellsBuffer_), bufferInfo(visitedBuffer_), bufferInfo(controlBuffer_)
    }};
    std::array<VkWriteDescriptorSet, 3> gridWrites{};
    for (uint32_t binding = 0; binding < gridWrites.size(); ++binding) {
        gridWrites[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gridWrites[binding].dstSet = gridDescriptorSet_;
        gridWrites[binding].dstBinding = binding;
        gridWrites[binding].descriptorCount = 1;
        gridWrites[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        gridWrites[binding].pBufferInfo = &gridInfos[binding];
    }
    const std::array<VkDescriptorBufferInfo, 5> computeInfos{{
        bufferInfo(cellsBuffer_), bufferInfo(distancesBuffer_), bufferInfo(previousBuffer_),
        bufferInfo(visitedBuffer_), bufferInfo(controlBuffer_)
    }};
    std::array<VkWriteDescriptorSet, 5> computeWrites{};
    for (uint32_t binding = 0; binding < computeWrites.size(); ++binding) {
        computeWrites[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        computeWrites[binding].dstSet = computeDescriptorSet_;
        computeWrites[binding].dstBinding = binding;
        computeWrites[binding].descriptorCount = 1;
        computeWrites[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        computeWrites[binding].pBufferInfo = &computeInfos[binding];
    }
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(gridWrites.size()), gridWrites.data(), 0, nullptr);
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(computeWrites.size()), computeWrites.data(), 0, nullptr);
}

void VulkanApp::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    throwIfFailed(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailable_), "Could not create image semaphore");
    throwIfFailed(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinished_), "Could not create render semaphore");
    throwIfFailed(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFence_), "Could not create frame fence");
}

void VulkanApp::waitForFrame() {
    throwIfFailed(vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, UINT64_MAX), "Could not wait for frame");
}

void VulkanApp::drawFrame() {
    uint32_t imageIndex = 0;
    const VkResult acquire = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageAvailable_, VK_NULL_HANDLE, &imageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Could not acquire swapchain image.");
    }

    const bool resetFirst = pendingReset_;
    const uint32_t requestedSteps = running_ ? stepsPerFrame_ : pendingSteps_;
    const uint32_t computeSteps = std::min(requestedSteps, safeStepsPerFrame());
    throwIfFailed(vkResetFences(device_, 1, &inFlightFence_), "Could not reset frame fence");
    throwIfFailed(vkResetCommandBuffer(commandBuffer_, 0), "Could not reset command buffer");
    recordCommandBuffer(imageIndex, computeSteps, resetFirst);

    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &imageAvailable_;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer_;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &renderFinished_;
    throwIfFailed(vkQueueSubmit(graphicsQueue_, 1, &submit, inFlightFence_), "Could not submit frame");
    pendingReset_ = false;
    pendingSteps_ = 0;

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &renderFinished_;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &imageIndex;
    const VkResult result = vkQueuePresentKHR(presentQueue_, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Could not present frame.");
    }
}

void VulkanApp::recordCommandBuffer(const uint32_t imageIndex, const uint32_t computeSteps, const bool resetFirst) {
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    throwIfFailed(vkBeginCommandBuffer(commandBuffer_, &begin), "Could not begin command buffer");

    VkMemoryBarrier hostBarrier{};
    hostBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    hostBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    hostBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(
        commandBuffer_,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 1, &hostBarrier, 0, nullptr, 0, nullptr);

    const uint32_t groups = (model_->nodeCount() + 255U) / 256U;
    if (resetFirst || computeSteps > 0U) {
        vkCmdBindDescriptorSets(
            commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout_,
            0, 1, &computeDescriptorSet_, 0, nullptr);
    }
    if (resetFirst) {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, resetPipeline_);
        vkCmdDispatch(commandBuffer_, groups, 1, 1);
        recordComputeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
    }
    for (uint32_t step = 0; step < computeSteps; ++step) {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, clearPipeline_);
        vkCmdDispatch(commandBuffer_, 1, 1, 1);
        recordComputeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, findMinPipeline_);
        vkCmdDispatch(commandBuffer_, groups, 1, 1);
        recordComputeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, chooseMinPipeline_);
        vkCmdDispatch(commandBuffer_, groups, 1, 1);
        recordComputeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, relaxPipeline_);
        vkCmdDispatch(commandBuffer_, groups, 1, 1);
        recordComputeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
    }

    if (resetFirst || computeSteps > 0U) {
        VkMemoryBarrier computeDone{};
        computeDone.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        computeDone.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        computeDone.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(
            commandBuffer_,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
            0, 1, &computeDone, 0, nullptr, 0, nullptr);
    }

    VkClearValue clear{};
    clear.color = {{kClear[0], kClear[1], kClear[2], kClear[3]}};
    VkRenderPassBeginInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderInfo.renderPass = renderPass_;
    renderInfo.framebuffer = framebuffers_[imageIndex];
    renderInfo.renderArea.extent = swapchainExtent_;
    renderInfo.clearValueCount = 1;
    renderInfo.pClearValues = &clear;
    vkCmdBeginRenderPass(commandBuffer_, &renderInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkDeviceSize zero = 0;
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipeline_);
    vkCmdBindDescriptorSets(
        commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout_,
        0, 1, &gridDescriptorSet_, 0, nullptr);
    vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &gridVertexBuffer_.handle, &zero);
    vkCmdDraw(commandBuffer_, 6, 1, 0, 0);

    if (!uiVertices_.empty()) {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, uiPipeline_);
        vkCmdBindVertexBuffers(commandBuffer_, 0, 1, &uiVertexBuffer_.handle, &zero);
        for (const DrawItem& item : uiDrawItems_) {
            vkCmdPushConstants(
                commandBuffer_, uiPipelineLayout_, VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(float) * 4U, item.color.data());
            vkCmdDraw(commandBuffer_, item.vertexCount, 1, item.firstVertex, 0);
        }
    }
    vkCmdEndRenderPass(commandBuffer_);
    throwIfFailed(vkEndCommandBuffer(commandBuffer_), "Could not finish command buffer");
}

void VulkanApp::recordComputeBarrier(const VkAccessFlags sourceAccess, const VkAccessFlags destinationAccess) {
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = sourceAccess;
    barrier.dstAccessMask = destinationAccess;
    vkCmdPipelineBarrier(
        commandBuffer_,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void VulkanApp::processInput() {
    if (keyPressedOnce(GLFW_KEY_SPACE)) {
        if (control_->finished != 0U) resetSearch("LISTO");
        running_ = !running_;
        status_ = running_ ? "EJECUTANDO EN GPU" : "PAUSADO";
    }
    if (keyPressedOnce(GLFW_KEY_N)) {
        if (control_->finished != 0U) resetSearch("LISTO");
        running_ = false;
        ++pendingSteps_;
        status_ = "PASO GPU";
    }
    if (keyPressedOnce(GLFW_KEY_R)) resetSearch("BUSQUEDA REINICIADA");
    if (keyPressedOnce(GLFW_KEY_C)) {
        model_->clearWalls();
        uploadModel();
        resetSearch("MAPA LIMPIO");
    }
    if (keyPressedOnce(GLFW_KEY_M)) {
        const auto seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
        model_->randomizeWalls(seed);
        uploadModel();
        resetSearch("MAPA ALEATORIO");
    }
    if (keyPressedOnce(GLFW_KEY_1)) tool_ = PaintTool::Source;
    if (keyPressedOnce(GLFW_KEY_2)) tool_ = PaintTool::Target;
    if (keyPressedOnce(GLFW_KEY_3)) tool_ = PaintTool::Wall;
    if (keyPressedOnce(GLFW_KEY_4)) tool_ = PaintTool::Weight1;
    if (keyPressedOnce(GLFW_KEY_5)) tool_ = PaintTool::Weight2;
    if (keyPressedOnce(GLFW_KEY_6)) tool_ = PaintTool::Weight5;

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window_, &mouseX, &mouseY);
    int windowWidth = 1;
    int windowHeight = 1;
    glfwGetWindowSize(window_, &windowWidth, &windowHeight);
    const float logicalX = static_cast<float>(mouseX * kLogicalWidth / std::max(1, windowWidth));
    const float logicalY = static_cast<float>(mouseY * kLogicalHeight / std::max(1, windowHeight));
    const bool leftDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (leftDown && !leftMouseWasDown_ && logicalX >= 520.0f) {
        handleSidebarClick(logicalX, logicalY);
    } else if (leftDown && logicalX < kCanvas && logicalY < kCanvas) {
        const std::optional<uint32_t> cell = cellAtCursor(mouseX, mouseY);
        if (cell.has_value() && (!leftMouseWasDown_ || cell.value() != lastPaintedCell_)) {
            paintCell(cell.value());
            lastPaintedCell_ = cell.value();
        }
    }
    if (!leftDown) lastPaintedCell_ = kInvalidNode;
    leftMouseWasDown_ = leftDown;
}

void VulkanApp::handleSidebarClick(const float x, const float y) {
    const Rect start{540, 58, 700, 96};
    const Rect step{720, 58, 880, 96};
    const Rect reset{540, 108, 700, 146};
    const Rect random{720, 108, 880, 146};
    const Rect clear{540, 158, 880, 196};
    if (contains(start, x, y)) {
        if (control_->finished != 0U) resetSearch("LISTO");
        running_ = !running_;
        status_ = running_ ? "EJECUTANDO EN GPU" : "PAUSADO";
        return;
    }
    if (contains(step, x, y)) {
        if (control_->finished != 0U) resetSearch("LISTO");
        running_ = false;
        ++pendingSteps_;
        status_ = "PASO GPU";
        return;
    }
    if (contains(reset, x, y)) {
        resetSearch("BUSQUEDA REINICIADA");
        return;
    }
    if (contains(random, x, y)) {
        const auto seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
        model_->randomizeWalls(seed);
        uploadModel();
        resetSearch("MAPA ALEATORIO");
        return;
    }
    if (contains(clear, x, y)) {
        model_->clearWalls();
        uploadModel();
        resetSearch("MAPA LIMPIO");
        return;
    }

    for (uint32_t index = 0; index < 6; ++index) {
        const Rect row{540.0f, 252.0f + index * 44.0f, 880.0f, 288.0f + index * 44.0f};
        if (contains(row, x, y)) {
            tool_ = static_cast<PaintTool>(index);
            status_ = "HERRAMIENTA SELECCIONADA";
            return;
        }
    }
    const std::array<Rect, 3> speedButtons{{
        {540, 570, 642, 608}, {659, 570, 761, 608}, {778, 570, 880, 608}
    }};
    const uint32_t speeds[] = {1U, 8U, 64U};
    for (std::size_t index = 0; index < speedButtons.size(); ++index) {
        if (contains(speedButtons[index], x, y)) {
            stepsPerFrame_ = speeds[index];
            const uint32_t effective = std::min(stepsPerFrame_, safeStepsPerFrame());
            status_ = effective < stepsPerFrame_
                ? "VELOCIDAD LIMITADA A X" + std::to_string(effective) + " POR SEGURIDAD"
                : "VELOCIDAD X" + std::to_string(effective);
            return;
        }
    }
}

void VulkanApp::paintCell(const uint32_t index) {
    switch (tool_) {
        case PaintTool::Source: model_->setSource(index); break;
        case PaintTool::Target: model_->setTarget(index); break;
        case PaintTool::Wall: model_->setWeight(index, 0U); break;
        case PaintTool::Weight1: model_->setWeight(index, 1U); break;
        case PaintTool::Weight2: model_->setWeight(index, 2U); break;
        case PaintTool::Weight5: model_->setWeight(index, 5U); break;
    }
    uploadCell(index);
    resetSearch("MAPA EDITADO");
}

void VulkanApp::resetSearch(const std::string& status) {
    running_ = false;
    pendingReset_ = true;
    pendingSteps_ = 0;
    resultConsumed_ = false;
    status_ = status;
    control_->source = model_->source();
    control_->target = model_->target();
    control_->width = model_->size().width;
    control_->height = model_->size().height;
    control_->finished = 0U;
    control_->found = 0U;
    control_->iterations = 0U;
}

void VulkanApp::uploadModel() {
    static_assert(std::endian::native == std::endian::little, "Packed cell uploads require a little-endian host.");
    const std::vector<uint8_t>& weights = model_->weights();
    std::memcpy(packedCells_, weights.data(), weights.size());
    auto* packedBytes = static_cast<uint8_t*>(cellsBuffer_.mapped);
    for (std::size_t index = weights.size(); index < cellsBuffer_.size; ++index) {
        packedBytes[index] = 0U;
    }
    control_->source = model_->source();
    control_->target = model_->target();
    control_->width = model_->size().width;
    control_->height = model_->size().height;
}

void VulkanApp::uploadCell(const uint32_t index) {
    const uint32_t shift = (index & 3U) * 8U;
    uint32_t& word = packedCells_[index >> 2U];
    word = (word & ~(0xffU << shift)) | (static_cast<uint32_t>(model_->weight(index)) << shift);
    control_->source = model_->source();
    control_->target = model_->target();
}

void VulkanApp::consumeGpuResult() {
    if (pendingReset_ || resultConsumed_ || control_ == nullptr || control_->finished == 0U) return;
    running_ = false;
    resultConsumed_ = true;
    if (control_->found == 0U) {
        status_ = "SIN RUTA";
        if (autoVerify_) {
            std::cout << "GPU verification failed: no route found\n";
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
        return;
    }
    rebuildPath();
    constexpr uint32_t kCpuValidationNodeLimit = 5'000'000U;
    if (model_->nodeCount() > kCpuValidationNodeLimit) {
        status_ = "RUTA GPU COMPLETA";
        if (autoVerify_) {
            std::cout << "GPU run completed without CPU validation: cost=" << distances_[model_->target()]
                      << ", visited=" << control_->iterations << '\n';
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
        return;
    }
    const DijkstraResult cpu = model_->solveCpu();
    if (cpu.found && distances_[model_->target()] == cpu.distance) {
        status_ = "RUTA GPU VALIDADA";
        if (autoVerify_) {
            std::cout << "GPU verification passed: cost=" << cpu.distance
                      << ", visited=" << control_->iterations << '\n';
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    } else {
        status_ = "DIFERENCIA CPU GPU";
        if (autoVerify_) {
            std::cout << "GPU verification failed: CPU/GPU mismatch\n";
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }
}

void VulkanApp::rebuildPath() {
    uint32_t current = model_->target();
    for (uint32_t count = 0; count < model_->nodeCount(); ++count) {
        const uint32_t shift = (current & 15U) * 2U;
        uint32_t& word = packedVisited_[current >> 4U];
        word = (word & ~(3U << shift)) | (2U << shift);
        if (current == model_->source()) return;
        current = previous_[current];
        if (current == kInvalidNode || current >= model_->nodeCount()) {
            status_ = "RUTA GPU INVALIDA";
            return;
        }
    }
    status_ = "CICLO EN PREDECESORES";
}

void VulkanApp::rebuildUi() {
    uiVertices_.clear();
    uiDrawItems_.clear();
    appendRect({0, 500, 500, 700}, kPanel);
    appendRect({500, 0, 520, 700}, kClear);
    appendRect({520, 0, 900, 700}, kSidebar);
    appendRect({520, 0, 900, 5}, kCyan);

    appendText("DIJKSTRA VULKAN", 540, 20, 4.0f, kText);
    appendText("VULKAN COMPUTE GPU", 540, 49, 1.0f, kCyan);

    const auto button = [&](const Rect rect, const std::string& label, const bool selected = false) {
        appendRect(rect, selected ? kButtonSelected : kButton);
        const float textWidth = static_cast<float>(label.size()) * 12.0f;
        appendText(label, (rect.minX + rect.maxX - textWidth) * 0.5f, rect.minY + 12.0f, 2.0f, kText);
    };
    button({540, 58, 700, 96}, running_ ? "PAUSAR" : "INICIAR", running_);
    button({720, 58, 880, 96}, "PASO");
    button({540, 108, 700, 146}, "REINICIAR");
    button({720, 108, 880, 146}, "ALEATORIO");
    button({540, 158, 880, 196}, "LIMPIAR MAPA");

    appendText("HERRAMIENTAS", 540, 220, 3.0f, kMuted);
    const std::array<std::string, 6> toolLabels{{
        "1 ORIGEN", "2 DESTINO", "3 MURO", "4 PESO 1", "5 PESO 2", "6 PESO 5"
    }};
    for (uint32_t index = 0; index < toolLabels.size(); ++index) {
        const Rect row{540.0f, 252.0f + index * 44.0f, 880.0f, 288.0f + index * 44.0f};
        button(row, toolLabels[index], static_cast<uint32_t>(tool_) == index);
        const std::array<float, 4> swatch =
            index == 0 ? kGreen : index == 1 ? kPink : index == 2 ? kClear :
            index == 4 ? std::array<float, 4>{{0.12f, 0.23f, 0.34f, 1.0f}} :
            index == 5 ? std::array<float, 4>{{0.25f, 0.13f, 0.34f, 1.0f}} : kPanel;
        appendRect({550.0f, row.minY + 9.0f, 568.0f, row.minY + 27.0f}, swatch);
    }

    appendText("PASOS POR FRAME MAX " + std::to_string(safeStepsPerFrame()), 540, 542, 1.65f, kMuted);
    button({540, 570, 642, 608}, "X1", stepsPerFrame_ == 1U);
    button({659, 570, 761, 608}, "X8", stepsPerFrame_ == 8U);
    button({778, 570, 880, 608}, "X64", stepsPerFrame_ == 64U);
    appendRect({540, 629, 556, 645}, kCyan);
    appendText("VISITADO", 566, 631, 2.0f, kMuted);
    appendRect({676, 629, 692, 645}, kGold);
    appendText("RUTA", 702, 631, 2.0f, kMuted);
    appendText("SPACE INICIA  N PASO  R RESET", 540, 666, 1.5f, kMuted);

    appendText("DIJKSTRA SOBRE GRILLA", 18, 526, 3.0f, kText);
    appendText("ESTADO: " + status_, 18, 559, 2.5f, status_ == "SIN RUTA" ? kPink : kCyan);
    const uint32_t iterations = control_ != nullptr ? control_->iterations : 0U;
    std::ostringstream stats;
    stats << "NODOS " << model_->nodeCount() << "  VISITADOS " << iterations;
    appendText(stats.str(), 18, 590, 2.0f, kText);
    std::ostringstream endpoints;
    endpoints << "ORIGEN " << model_->source() << "  DESTINO " << model_->target();
    appendText(endpoints.str(), 18, 618, 2.0f, kMuted);
    if (control_ != nullptr && control_->found != 0U && distances_[model_->target()] != kInvalidNode) {
        appendText("COSTO TOTAL " + std::to_string(distances_[model_->target()]), 18, 646, 2.0f, kGold);
    } else {
        appendText("PINTA EL MAPA Y PRESIONA INICIAR", 18, 646, 2.0f, kMuted);
    }
    appendText("VERDE ORIGEN  ROSA DESTINO", 18, 674, 1.7f, kMuted);

    if (uiVertices_.size() > kUiVertexCapacity) {
        throw std::runtime_error("UI vertex capacity exceeded.");
    }
    std::memcpy(uiVertexBuffer_.mapped, uiVertices_.data(), uiVertices_.size() * sizeof(UiVertex));
}

void VulkanApp::refreshTitle() const {
    std::ostringstream title;
    title << "DijkstraVulkan | " << model_->size().width << 'x' << model_->size().height
          << " | " << (running_ ? "GPU RUN" : status_);
    glfwSetWindowTitle(window_, title.str().c_str());
}

void VulkanApp::appendRect(const Rect rect, const std::array<float, 4>& color) {
    const uint32_t first = static_cast<uint32_t>(uiVertices_.size());
    uiVertices_.push_back({{ndcX(rect.minX), ndcY(rect.minY)}});
    uiVertices_.push_back({{ndcX(rect.maxX), ndcY(rect.minY)}});
    uiVertices_.push_back({{ndcX(rect.maxX), ndcY(rect.maxY)}});
    uiVertices_.push_back({{ndcX(rect.minX), ndcY(rect.minY)}});
    uiVertices_.push_back({{ndcX(rect.maxX), ndcY(rect.maxY)}});
    uiVertices_.push_back({{ndcX(rect.minX), ndcY(rect.maxY)}});
    uiDrawItems_.push_back({first, 6U, color});
}

void VulkanApp::appendText(
    const std::string& text,
    const float x,
    const float y,
    const float pixelSize,
    const std::array<float, 4>& color) {
    const uint32_t first = static_cast<uint32_t>(uiVertices_.size());
    float cursor = x;
    for (const char raw : text) {
        const char value = raw >= 'a' && raw <= 'z' ? static_cast<char>(raw - 'a' + 'A') : raw;
        const auto rows = glyph(value);
        for (uint32_t row = 0; row < rows.size(); ++row) {
            for (uint32_t column = 0; column < 5; ++column) {
                if ((rows[row] & (1U << (4U - column))) == 0U) continue;
                const float minX = cursor + column * pixelSize;
                const float minY = y + row * pixelSize;
                uiVertices_.push_back({{ndcX(minX), ndcY(minY)}});
                uiVertices_.push_back({{ndcX(minX + pixelSize), ndcY(minY)}});
                uiVertices_.push_back({{ndcX(minX + pixelSize), ndcY(minY + pixelSize)}});
                uiVertices_.push_back({{ndcX(minX), ndcY(minY)}});
                uiVertices_.push_back({{ndcX(minX + pixelSize), ndcY(minY + pixelSize)}});
                uiVertices_.push_back({{ndcX(minX), ndcY(minY + pixelSize)}});
            }
        }
        cursor += (value == ' ' ? 4.0f : 6.0f) * pixelSize;
    }
    const uint32_t count = static_cast<uint32_t>(uiVertices_.size()) - first;
    if (count > 0U) uiDrawItems_.push_back({first, count, color});
}

bool VulkanApp::contains(const Rect rect, const float x, const float y) const {
    return x >= rect.minX && x <= rect.maxX && y >= rect.minY && y <= rect.maxY;
}

std::optional<uint32_t> VulkanApp::cellAtCursor(const double mouseX, const double mouseY) const {
    int width = 1;
    int height = 1;
    glfwGetWindowSize(window_, &width, &height);
    const double x = mouseX * kLogicalWidth / std::max(1, width);
    const double y = mouseY * kLogicalHeight / std::max(1, height);
    if (x < 0.0 || y < 0.0 || x >= kCanvas || y >= kCanvas) return std::nullopt;
    const uint32_t cellX = std::min(
        static_cast<uint32_t>(x / kCanvas * model_->size().width), model_->size().width - 1U);
    const uint32_t cellY = std::min(
        static_cast<uint32_t>(y / kCanvas * model_->size().height), model_->size().height - 1U);
    return cellY * model_->size().width + cellX;
}

bool VulkanApp::keyPressedOnce(const int key) {
    const bool down = glfwGetKey(window_, key) == GLFW_PRESS;
    const bool pressed = down && !keyLatch_[key];
    keyLatch_[key] = down;
    return pressed;
}

uint32_t VulkanApp::safeStepsPerFrame() const {
    if (!model_) return 1U;

    // Each Dijkstra step performs three full-grid compute passes. Keep their
    // aggregate work bounded so the graphics queue can continue servicing the
    // desktop even when the user selects X64 on a large grid.
    constexpr uint64_t kShaderInvocationBudget = 12'000'000ULL;
    constexpr uint64_t kFullGridPassesPerStep = 3ULL;
    const uint64_t invocationsPerStep = std::max<uint64_t>(
        1ULL, static_cast<uint64_t>(model_->nodeCount()) * kFullGridPassesPerStep);
    return static_cast<uint32_t>(std::clamp<uint64_t>(
        kShaderInvocationBudget / invocationsPerStep, 1ULL, 64ULL));
}

VulkanApp::QueueFamilies VulkanApp::findQueueFamilies(const VkPhysicalDevice device) const {
    QueueFamilies result{};
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());
    for (uint32_t index = 0; index < count; ++index) {
        const VkQueueFlags flags = properties[index].queueFlags;
        if ((flags & VK_QUEUE_GRAPHICS_BIT) != 0U && (flags & VK_QUEUE_COMPUTE_BIT) != 0U) {
            result.graphicsCompute = index;
        }
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface_, &present);
        if (present == VK_TRUE) result.present = index;
        if (result.complete()) break;
    }
    return result;
}

VulkanApp::SwapSupport VulkanApp::querySwapSupport(const VkPhysicalDevice device) const {
    SwapSupport support{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &support.capabilities);
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &count, nullptr);
    support.formats.resize(count);
    if (count > 0U) vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &count, support.formats.data());
    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &count, nullptr);
    support.presentModes.resize(count);
    if (count > 0U) vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &count, support.presentModes.data());
    return support;
}

bool VulkanApp::deviceSupportsSwapchain(const VkPhysicalDevice device) const {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
    return std::any_of(extensions.begin(), extensions.end(), [](const VkExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
    });
}

VkSurfaceFormatKHR VulkanApp::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const {
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return format;
    }
    return formats.front();
}

VkPresentModeKHR VulkanApp::choosePresentMode(const std::vector<VkPresentModeKHR>&) const {
    // FIFO is guaranteed by Vulkan and naturally follows the display refresh.
    // MAILBOX allowed this single-frame loop to submit work as fast as possible,
    // which could needlessly saturate the GPU even while the search was paused.
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApp::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    return {
        std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t VulkanApp::findMemoryType(const uint32_t typeBits, const VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        if ((typeBits & (1U << index)) != 0U &&
            (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties) return index;
    }
    throw std::runtime_error("No compatible Vulkan memory type was found.");
}

VkShaderModule VulkanApp::createShaderModule(const std::filesystem::path& path) const {
    const std::vector<char> code = readBinary(path);
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module = VK_NULL_HANDLE;
    throwIfFailed(vkCreateShaderModule(device_, &info, nullptr, &module), "Could not create shader module");
    return module;
}

std::filesystem::path VulkanApp::shaderPath(const std::string& name) const {
    try {
        const std::filesystem::path executable = std::filesystem::read_symlink("/proc/self/exe");
        const auto candidate = executable.parent_path() / "shaders" / name;
        if (std::filesystem::exists(candidate)) return candidate;
    } catch (...) {
    }
    return std::filesystem::current_path() / "shaders" / name;
}

void VulkanApp::createBuffer(
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags properties,
    Buffer& buffer) {
    buffer.size = size;
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    throwIfFailed(vkCreateBuffer(device_, &info, nullptr, &buffer.handle), "Could not create buffer");
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, buffer.handle, &requirements);
    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties);
    throwIfFailed(vkAllocateMemory(device_, &allocation, nullptr, &buffer.memory), "Could not allocate buffer memory");
    throwIfFailed(vkBindBufferMemory(device_, buffer.handle, buffer.memory, 0), "Could not bind buffer memory");
    if ((properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0U) {
        throwIfFailed(vkMapMemory(device_, buffer.memory, 0, size, 0, &buffer.mapped), "Could not map buffer memory");
    }
}

void VulkanApp::destroyBuffer(Buffer& buffer) {
    if (device_ == VK_NULL_HANDLE) return;
    if (buffer.mapped != nullptr) vkUnmapMemory(device_, buffer.memory);
    if (buffer.handle != VK_NULL_HANDLE) vkDestroyBuffer(device_, buffer.handle, nullptr);
    if (buffer.memory != VK_NULL_HANDLE) vkFreeMemory(device_, buffer.memory, nullptr);
    buffer = {};
}

VkPipeline VulkanApp::createComputePipeline(const std::string& shaderName) const {
    const VkShaderModule shader = createShaderModule(shaderPath(shaderName));
    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = shader;
    stage.pName = "main";
    VkComputePipelineCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    info.stage = stage;
    info.layout = computePipelineLayout_;
    VkPipeline pipeline = VK_NULL_HANDLE;
    const VkResult result = vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
    vkDestroyShaderModule(device_, shader, nullptr);
    throwIfFailed(result, "Could not create compute pipeline");
    return pipeline;
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int, int) {
    auto* app = static_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) app->framebufferResized_ = true;
}
