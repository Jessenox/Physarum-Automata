#include "presentation/VulkanApp.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

constexpr float kLogicalWidth = 1000.0F;
constexpr float kLogicalHeight = 720.0F;
constexpr std::array<float, 4> kClear{{0.025F, 0.030F, 0.040F, 1.0F}};
constexpr std::array<float, 4> kPanel{{0.090F, 0.105F, 0.130F, 1.0F}};
constexpr std::array<float, 4> kSidebar{{0.055F, 0.065F, 0.085F, 1.0F}};
constexpr std::array<float, 4> kButton{{0.145F, 0.175F, 0.220F, 1.0F}};
constexpr std::array<float, 4> kSelected{{0.10F, 0.42F, 0.48F, 1.0F}};
constexpr std::array<float, 4> kText{{0.89F, 0.92F, 0.96F, 1.0F}};
constexpr std::array<float, 4> kMuted{{0.52F, 0.59F, 0.68F, 1.0F}};
constexpr std::array<float, 4> kCyan{{0.05F, 0.78F, 0.92F, 1.0F}};
constexpr std::array<float, 4> kGreen{{0.18F, 0.92F, 0.48F, 1.0F}};
constexpr std::array<float, 4> kPink{{0.98F, 0.25F, 0.65F, 1.0F}};
constexpr std::array<float, 4> kGold{{1.0F, 0.72F, 0.16F, 1.0F}};
constexpr std::array<float, 4> kPurple{{0.62F, 0.38F, 0.96F, 1.0F}};
constexpr float kPi = 3.14159265358979323846F;

float ndcX(const float x) { return (x / kLogicalWidth) * 2.0F - 1.0F; }
float ndcY(const float y) { return (y / kLogicalHeight) * 2.0F - 1.0F; }

void throwIfFailed(const VkResult result, const std::string_view action) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(action) + " (VkResult=" + std::to_string(result) + ")");
    }
}

std::vector<char> readBinary(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) throw std::runtime_error("Could not open shader: " + path.string());
    const std::streamsize size = file.tellg();
    std::vector<char> data(static_cast<std::size_t>(size));
    file.seekg(0);
    if (size > 0 && !file.read(data.data(), size)) throw std::runtime_error("Could not read shader: " + path.string());
    return data;
}

std::array<uint8_t, 7> glyph(const char value) {
    switch (value) {
        case '0': return {{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}};
        case '1': return {{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}};
        case '2': return {{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}};
        case '3': return {{0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}};
        case '4': return {{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}};
        case '5': return {{0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}};
        case '6': return {{0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}};
        case '7': return {{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}};
        case '8': return {{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}};
        case '9': return {{0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}};
        case 'A': return {{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}};
        case 'B': return {{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}};
        case 'C': return {{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}};
        case 'D': return {{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}};
        case 'E': return {{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}};
        case 'F': return {{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}};
        case 'G': return {{0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}};
        case 'H': return {{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}};
        case 'I': return {{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}};
        case 'J': return {{0x01,0x01,0x01,0x01,0x11,0x11,0x0E}};
        case 'K': return {{0x11,0x12,0x14,0x18,0x14,0x12,0x11}};
        case 'L': return {{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}};
        case 'M': return {{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}};
        case 'N': return {{0x11,0x19,0x15,0x13,0x11,0x11,0x11}};
        case 'O': return {{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}};
        case 'P': return {{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}};
        case 'Q': return {{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}};
        case 'R': return {{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}};
        case 'S': return {{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}};
        case 'T': return {{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}};
        case 'U': return {{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}};
        case 'V': return {{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}};
        case 'W': return {{0x11,0x11,0x11,0x15,0x15,0x1B,0x11}};
        case 'X': return {{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}};
        case 'Y': return {{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}};
        case 'Z': return {{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}};
        case ':': return {{0x00,0x04,0x04,0x00,0x04,0x04,0x00}};
        case '.': return {{0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}};
        case '/': return {{0x01,0x02,0x02,0x04,0x08,0x08,0x10}};
        case '-': return {{0x00,0x00,0x00,0x1F,0x00,0x00,0x00}};
        case '=': return {{0x00,0x1F,0x00,0x1F,0x00,0x00,0x00}};
        default: return {{0,0,0,0,0,0,0}};
    }
}

uint32_t optimalGridCost(const comparison::Scenario& scenario, const uint32_t goal) {
    const auto heuristic = [&](const uint32_t node) {
        const uint32_t x0 = node % scenario.width;
        const uint32_t y0 = node / scenario.width;
        const uint32_t x1 = goal % scenario.width;
        const uint32_t y1 = goal / scenario.width;
        return (x0 > x1 ? x0 - x1 : x1 - x0) + (y0 > y1 ? y0 - y1 : y1 - y0);
    };
    if (scenario.obstacles.empty()) return heuristic(scenario.source);
    using QueueItem = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<>> frontier;
    std::unordered_map<uint32_t, uint32_t> distance;
    distance.reserve(65'536U);
    distance.emplace(scenario.source, 0U);
    frontier.emplace(heuristic(scenario.source), heuristic(scenario.source), scenario.source, 0U);
    while (!frontier.empty()) {
        const auto [priority, remaining, current, currentCost] = frontier.top();
        (void)priority;
        (void)remaining;
        frontier.pop();
        const auto known = distance.find(current);
        if (known == distance.end() || known->second != currentCost) continue;
        if (current == goal) return currentCost;
        const uint32_t x = current % scenario.width;
        const uint32_t y = current / scenario.width;
        const uint32_t invalid = std::numeric_limits<uint32_t>::max();
        const uint32_t nextNodes[4] = {
            x > 0U ? current - 1U : invalid,
            x + 1U < scenario.width ? current + 1U : invalid,
            y > 0U ? current - scenario.width : invalid,
            y + 1U < scenario.height ? current + scenario.width : invalid,
        };
        for (const uint32_t next : nextNodes) {
            if (next == invalid || !scenario.traversable(next)) continue;
            const uint32_t nextCost = currentCost + 1U;
            const auto nextKnown = distance.find(next);
            if (nextKnown != distance.end() && nextKnown->second <= nextCost) continue;
            distance[next] = nextCost;
            const uint32_t nextHeuristic = heuristic(next);
            frontier.emplace(nextCost + nextHeuristic, nextHeuristic, next, nextCost);
        }
    }
    return 0U;
}

}  // namespace

VulkanApp::VulkanApp(comparison::Scenario scenario, const uint32_t iterations,
                     const bool autoVerify, const bool guidedMode, const bool odorEnabled)
    : scenario_(std::move(scenario)), autoVerify_(autoVerify), guidedMode_(guidedMode),
      odorEnabled_(odorEnabled),
      iterationLimit_(iterations) {
    scenario_.validate();
    if (iterationLimit_ == 0U || iterationLimit_ > kIterationCapacity) {
        throw std::invalid_argument("Iteration count must be between 1 and 10000.");
    }
    if (scenario_.width > 10'000U || scenario_.height > 10'000U) {
        throw std::invalid_argument("Implicit GPU grids are limited to 10000x10000.");
    }
    if (scenario_.goals.size() > kMaxGoals) throw std::invalid_argument("At most 30 goals are supported.");
    if (scenario_.obstacles.size() > kMaxObstacles) throw std::invalid_argument("Too many sparse obstacles.");
    cameraCenterX_ = static_cast<float>(scenario_.width) * 0.5F;
    cameraCenterY_ = static_cast<float>(scenario_.height) * 0.5F;
    cameraWidth_ = std::max(static_cast<float>(scenario_.width),
                            static_cast<float>(scenario_.height) * 570.0F / 455.0F);
    recalculateOptimalCosts();
    if (autoVerify_ && optimalCostTotal_ == 0U) {
        throw std::invalid_argument("At least one goal is disconnected from the start.");
    }
}
VulkanApp::~VulkanApp() { cleanup(); }

void VulkanApp::run() {
    initWindow();
    initVulkan();
    mainLoop();
}

void VulkanApp::initWindow() {
    if (glfwInit() != GLFW_TRUE) throw std::runtime_error("GLFW initialization failed.");
    if (glfwVulkanSupported() != GLFW_TRUE) throw std::runtime_error("GLFW did not find Vulkan.");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(kWindowWidth, kWindowHeight, "NetworkACOVulkan", nullptr, nullptr);
    if (window_ == nullptr) throw std::runtime_error("Could not create NetworkACOVulkan window.");
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetScrollCallback(window_, scrollCallback);
}

void VulkanApp::initVulkan() {
    createInstance();
    createSurface();
    pickPhysicalDevice();
    configureAntCount();
    createDevice();
    createSwapchain();
    createImageViews();
    createRenderPass();
    createDescriptorLayout();
    createPipelineLayouts();
    createGraphicsPipeline();
    createComputePipelines();
    createFramebuffers();
    createCommandResources();
    createBuffers();
    createDescriptors();
    createSyncObjects();
    resetSimulation();
    if (autoVerify_) {
        running_ = true;
        status_ = "VERIFICANDO GPU";
    }
    lastAnimationTime_ = glfwGetTime();
    rebuildUi();
    std::cout << "NetworkACOVulkan GPU: " << gpuName_ << '\n'
              << "Comparison grid: " << scenario_.width << 'x' << scenario_.height
              << ", sparse obstacles=" << scenario_.obstacles.size()
              << ", goals=" << scenario_.goals.size() << ", optimumTotal=" << optimalCostTotal_ << "\n"
              << "ACO: ants=" << antCount_;
    if (antCount_ != desiredAntCount_) std::cout << " (GPU limit, requested " << desiredAntCount_ << ')';
    std::cout << ", iterations=" << iterationLimit_
              << ", maxPath=" << control_->maxPath
              << ", visitedHash=" << control_->visitedCapacity << " slots/ant"
              << ", T1=discovery-bootstrap, T2+=independent-ACO-roulette"
              << ", mode=" << (guidedMode_ ? "guided" : "unguided")
              << ", odor=" << (odorEnabled_ ? "on" : "off")
              << ", odorRadius=" << control_->odorRadius
              << ", odorStrength=" << control_->odorStrength
              << ", sourceOdor=" << odorConcentrationAt(scenario_.source)
              << ", alpha=1, beta=2, rho=0.1, Q=100, seed=" << scenario_.seed << '\n';
}

void VulkanApp::mainLoop() {
    while (glfwWindowShouldClose(window_) == GLFW_FALSE) {
        glfwPollEvents();
        waitForFrame();
        consumeGpuResults();
        processInput();
        updateAntAnimation();
        rebuildUi();
        refreshTitle();
        drawFrame();
    }
    if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
}

void VulkanApp::cleanup() {
    if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
    cleanupSwapchain();
    for (VkPipeline pipeline : {initPipeline_, beginPipeline_, constructPipeline_, accumulatePipeline_,
                                advancePipeline_}) {
        if (pipeline != VK_NULL_HANDLE) vkDestroyPipeline(device_, pipeline, nullptr);
    }
    if (graphicsPipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, graphicsPipelineLayout_, nullptr);
    if (computePipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, computePipelineLayout_, nullptr);
    if (computeDescriptorLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device_, computeDescriptorLayout_, nullptr);
    if (descriptorPool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    for (Buffer& buffer : storageBuffers_) destroyBuffer(buffer);
    destroyBuffer(vertexBuffer_);
    if (inFlightFence_ != VK_NULL_HANDLE) vkDestroyFence(device_, inFlightFence_, nullptr);
    if (imageAvailable_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, imageAvailable_, nullptr);
    if (renderFinished_ != VK_NULL_HANDLE) vkDestroySemaphore(device_, renderFinished_, nullptr);
    if (commandPool_ != VK_NULL_HANDLE) vkDestroyCommandPool(device_, commandPool_, nullptr);
    if (device_ != VK_NULL_HANDLE) vkDestroyDevice(device_, nullptr);
    if (surface_ != VK_NULL_HANDLE) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    if (instance_ != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
    if (window_ != nullptr) glfwDestroyWindow(window_);
    glfwTerminate();
    device_ = VK_NULL_HANDLE;
    instance_ = VK_NULL_HANDLE;
    window_ = nullptr;
}

void VulkanApp::cleanupSwapchain() {
    if (device_ == VK_NULL_HANDLE) return;
    for (const VkFramebuffer framebuffer : framebuffers_) vkDestroyFramebuffer(device_, framebuffer, nullptr);
    framebuffers_.clear();
    if (graphicsPipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    graphicsPipeline_ = VK_NULL_HANDLE;
    if (renderPass_ != VK_NULL_HANDLE) vkDestroyRenderPass(device_, renderPass_, nullptr);
    renderPass_ = VK_NULL_HANDLE;
    for (const VkImageView view : swapchainImageViews_) vkDestroyImageView(device_, view, nullptr);
    swapchainImageViews_.clear();
    if (swapchain_ != VK_NULL_HANDLE) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
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
    createGraphicsPipeline();
    createFramebuffers();
}

void VulkanApp::createInstance() {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "NetworkACOVulkan";
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "No Engine";
    app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app.apiVersion = VK_API_VERSION_1_1;
    uint32_t count = 0U;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = count;
    info.ppEnabledExtensionNames = extensions;
    throwIfFailed(vkCreateInstance(&info, nullptr, &instance_), "vkCreateInstance");
}

void VulkanApp::createSurface() {
    throwIfFailed(glfwCreateWindowSurface(instance_, window_, nullptr, &surface_), "glfwCreateWindowSurface");
}

void VulkanApp::pickPhysicalDevice() {
    uint32_t count = 0U;
    vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (count == 0U) throw std::runtime_error("No Vulkan physical device found.");
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    int bestRank = -1;
    for (const VkPhysicalDevice candidate : devices) {
        const QueueFamilies families = findQueueFamilies(candidate);
        if (!families.complete() || !deviceSupportsSwapchain(candidate)) continue;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        const int rank = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 3
                       : properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 2 : 1;
        if (rank > bestRank) {
            bestRank = rank;
            physicalDevice_ = candidate;
            queueFamilies_ = families;
            physicalDeviceProperties_ = properties;
        }
    }
    if (physicalDevice_ == VK_NULL_HANDLE) throw std::runtime_error("No graphics+compute Vulkan device with swapchain support.");
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties_);
    gpuName_ = physicalDeviceProperties_.deviceName;
}

void VulkanApp::createDevice() {
    const std::set<uint32_t> families{*queueFamilies_.graphicsCompute, *queueFamilies_.present};
    const float priority = 1.0F;
    std::vector<VkDeviceQueueCreateInfo> queues;
    for (const uint32_t family : families) {
        VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue.queueFamilyIndex = family;
        queue.queueCount = 1U;
        queue.pQueuePriorities = &priority;
        queues.push_back(queue);
    }
    const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures features{};
    VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    info.pQueueCreateInfos = queues.data();
    info.enabledExtensionCount = 1U;
    info.ppEnabledExtensionNames = extensions;
    info.pEnabledFeatures = &features;
    throwIfFailed(vkCreateDevice(physicalDevice_, &info, nullptr, &device_), "vkCreateDevice");
    vkGetDeviceQueue(device_, *queueFamilies_.graphicsCompute, 0U, &graphicsQueue_);
    vkGetDeviceQueue(device_, *queueFamilies_.present, 0U, &presentQueue_);
}

void VulkanApp::createSwapchain() {
    const SwapSupport support = querySwapSupport(physicalDevice_);
    const VkSurfaceFormatKHR format = chooseSurfaceFormat(support.formats);
    const VkPresentModeKHR presentMode = choosePresentMode(support.presentModes);
    const VkExtent2D extent = chooseExtent(support.capabilities);
    uint32_t count = support.capabilities.minImageCount + 1U;
    if (support.capabilities.maxImageCount > 0U) count = std::min(count, support.capabilities.maxImageCount);
    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = surface_;
    info.minImageCount = count;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1U;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    const uint32_t families[] = {*queueFamilies_.graphicsCompute, *queueFamilies_.present};
    if (families[0] != families[1]) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2U;
        info.pQueueFamilyIndices = families;
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    info.preTransform = support.capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = presentMode;
    info.clipped = VK_TRUE;
    throwIfFailed(vkCreateSwapchainKHR(device_, &info, nullptr, &swapchain_), "vkCreateSwapchainKHR");
    vkGetSwapchainImagesKHR(device_, swapchain_, &count, nullptr);
    swapchainImages_.resize(count);
    vkGetSwapchainImagesKHR(device_, swapchain_, &count, swapchainImages_.data());
    swapchainFormat_ = format.format;
    swapchainExtent_ = extent;
}

void VulkanApp::createImageViews() {
    swapchainImageViews_.resize(swapchainImages_.size());
    for (std::size_t index = 0U; index < swapchainImages_.size(); ++index) {
        VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        info.image = swapchainImages_[index];
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = swapchainFormat_;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1U;
        info.subresourceRange.layerCount = 1U;
        throwIfFailed(vkCreateImageView(device_, &info, nullptr, &swapchainImageViews_[index]), "vkCreateImageView");
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
    VkAttachmentReference reference{0U, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1U;
    subpass.pColorAttachments = &reference;
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0U;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    info.attachmentCount = 1U;
    info.pAttachments = &color;
    info.subpassCount = 1U;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1U;
    info.pDependencies = &dependency;
    throwIfFailed(vkCreateRenderPass(device_, &info, nullptr, &renderPass_), "vkCreateRenderPass");
}

void VulkanApp::createDescriptorLayout() {
    std::array<VkDescriptorSetLayoutBinding, 19> bindings{};
    for (uint32_t index = 0U; index < bindings.size(); ++index) {
        bindings[index].binding = index;
        bindings[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[index].descriptorCount = 1U;
        bindings[index].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();
    throwIfFailed(vkCreateDescriptorSetLayout(device_, &info, nullptr, &computeDescriptorLayout_),
                  "vkCreateDescriptorSetLayout");
}

void VulkanApp::createPipelineLayouts() {
    VkPipelineLayoutCreateInfo graphics{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    throwIfFailed(vkCreatePipelineLayout(device_, &graphics, nullptr, &graphicsPipelineLayout_),
                  "vkCreatePipelineLayout(graphics)");
    VkPipelineLayoutCreateInfo compute{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    compute.setLayoutCount = 1U;
    compute.pSetLayouts = &computeDescriptorLayout_;
    throwIfFailed(vkCreatePipelineLayout(device_, &compute, nullptr, &computePipelineLayout_),
                  "vkCreatePipelineLayout(compute)");
}

void VulkanApp::createGraphicsPipeline() {
    const VkShaderModule vertex = createShaderModule(shaderPath("ui.vert.spv"));
    const VkShaderModule fragment = createShaderModule(shaderPath("ui.frag.spv"));
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertex;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragment;
    stages[1].pName = "main";
    VkVertexInputBindingDescription binding{0U, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    std::array<VkVertexInputAttributeDescription, 2> attributes{{
        {0U, 0U, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, position))},
        {1U, 0U, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, color))},
    }};
    VkPipelineVertexInputStateCreateInfo vertexInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInput.vertexBindingDescriptionCount = 1U;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport viewport{0.0F, 0.0F, static_cast<float>(swapchainExtent_.width),
                        static_cast<float>(swapchainExtent_.height), 0.0F, 1.0F};
    VkRect2D scissor{{0, 0}, swapchainExtent_};
    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1U;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1U;
    viewportState.pScissors = &scissor;
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster.lineWidth = 1.0F;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState attachment{};
    attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                              | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    attachment.blendEnable = VK_TRUE;
    attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    attachment.colorBlendOp = VK_BLEND_OP_ADD;
    attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1U;
    blend.pAttachments = &attachment;
    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.stageCount = 2U;
    info.pStages = stages;
    info.pVertexInputState = &vertexInput;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewportState;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pColorBlendState = &blend;
    info.layout = graphicsPipelineLayout_;
    info.renderPass = renderPass_;
    const VkResult result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1U, &info, nullptr, &graphicsPipeline_);
    vkDestroyShaderModule(device_, fragment, nullptr);
    vkDestroyShaderModule(device_, vertex, nullptr);
    throwIfFailed(result, "vkCreateGraphicsPipelines");
}

void VulkanApp::createComputePipelines() {
    initPipeline_ = createComputePipeline("aco_init.comp.spv");
    beginPipeline_ = createComputePipeline("aco_begin.comp.spv");
    constructPipeline_ = createComputePipeline("aco_construct.comp.spv");
    accumulatePipeline_ = createComputePipeline("aco_accumulate.comp.spv");
    advancePipeline_ = createComputePipeline("aco_advance.comp.spv");
}

void VulkanApp::createFramebuffers() {
    framebuffers_.resize(swapchainImageViews_.size());
    for (std::size_t index = 0U; index < swapchainImageViews_.size(); ++index) {
        VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        info.renderPass = renderPass_;
        info.attachmentCount = 1U;
        info.pAttachments = &swapchainImageViews_[index];
        info.width = swapchainExtent_.width;
        info.height = swapchainExtent_.height;
        info.layers = 1U;
        throwIfFailed(vkCreateFramebuffer(device_, &info, nullptr, &framebuffers_[index]), "vkCreateFramebuffer");
    }
}

void VulkanApp::createCommandResources() {
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = *queueFamilies_.graphicsCompute;
    throwIfFailed(vkCreateCommandPool(device_, &pool, nullptr, &commandPool_), "vkCreateCommandPool");
    VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate.commandPool = commandPool_;
    allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate.commandBufferCount = 1U;
    throwIfFailed(vkAllocateCommandBuffers(device_, &allocate, &commandBuffer_), "vkAllocateCommandBuffers");
}

void VulkanApp::configureAntCount() {
    desiredAntCount_ = desiredAntCountForGrid();
    const VkDeviceSize storageRange = physicalDeviceProperties_.limits.maxStorageBufferRange;
    const VkDeviceSize matrixBytesPerAntSquared =
        static_cast<VkDeviceSize>(kIterationCapacity) * sizeof(float);
    const uint32_t matrixLimit = static_cast<uint32_t>(
        std::sqrt(static_cast<double>(storageRange / matrixBytesPerAntSquared)));
    const VkDeviceSize maxPathBytesPerAnt =
        static_cast<VkDeviceSize>(maxPathCapacityForGrid()) * sizeof(uint32_t);
    const uint32_t pathLimit = static_cast<uint32_t>(storageRange / maxPathBytesPerAnt);
    const VkDeviceSize visitedBytesPerAnt =
        static_cast<VkDeviceSize>(visitedCapacityForGrid()) * sizeof(uint32_t) * 2U;
    const uint32_t visitedLimit = static_cast<uint32_t>(storageRange / visitedBytesPerAnt);
    const VkDeviceSize pheromoneSlots = static_cast<VkDeviceSize>(std::min(scenario_.width, kPheromoneResolution))
        * std::min(scenario_.height, kPheromoneResolution) * 4U;
    const uint32_t pheromoneLimit = static_cast<uint32_t>(storageRange /
        (pheromoneSlots * sizeof(uint32_t)));
    const uint32_t deviceLimit = std::min(
        {kMaxAntCount, matrixLimit, pathLimit, visitedLimit, pheromoneLimit});
    if (deviceLimit < kBaseAntCount) {
        throw std::runtime_error("GPU storage-buffer limits cannot support the minimum 30-ant colony.");
    }
    antCount_ = std::min(desiredAntCount_, deviceLimit);
}

void VulkanApp::createBuffers() {
    const VkDeviceSize routeCount = static_cast<VkDeviceSize>(kIterationCapacity) * antCount_;
    const VkDeviceSize matrixSize = static_cast<VkDeviceSize>(antCount_) * antCount_;
    const VkDeviceSize maxPath = maxPathCapacityForGrid();
    const VkDeviceSize visitedCapacity = visitedCapacityForGrid();
    const VkDeviceSize tileWidth = std::min(scenario_.width, kPheromoneResolution);
    const VkDeviceSize tileHeight = std::min(scenario_.height, kPheromoneResolution);
    const VkDeviceSize pheromoneSlots = tileWidth * tileHeight * 4U;
    createBuffer(static_cast<VkDeviceSize>(kVertexCapacity) * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 vertexBuffer_);
    const std::array<VkDeviceSize, 19> sizes{{
        static_cast<VkDeviceSize>(kMaxObstacles) * sizeof(uint32_t),
        static_cast<VkDeviceSize>(kMaxGoals) * sizeof(uint32_t),
        static_cast<VkDeviceSize>(kMaxGoals) * sizeof(GpuGoalBest),
        pheromoneSlots * sizeof(uint32_t),
        pheromoneSlots * antCount_ * sizeof(uint32_t),
        antCount_ * maxPath * sizeof(uint32_t),
        routeCount * sizeof(uint32_t),
        routeCount * sizeof(float),
        routeCount * sizeof(uint32_t),
        matrixSize * sizeof(float),
        matrixSize * sizeof(float),
        kIterationCapacity * matrixSize * sizeof(float),
        sizeof(GpuControl),
        kMaxGoals * maxPath * sizeof(uint32_t),
        antCount_ * visitedCapacity * sizeof(uint32_t) * 2U,
        antCount_ * maxPath * sizeof(uint32_t),
        antCount_ * sizeof(uint32_t),
        antCount_ * sizeof(uint32_t) * 4U,
        antCount_ * sizeof(uint32_t),
    }};
    for (std::size_t index = 0U; index < storageBuffers_.size(); ++index) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        if (index == 14U) usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        createBuffer(sizes[index], usage, storageBuffers_[index]);
    }
    totalPheromone_ = static_cast<uint32_t*>(storageBuffers_[3].mapped);
    pheromoneByAnt_ = static_cast<uint32_t*>(storageBuffers_[4].mapped);
    pathNodes_ = static_cast<uint32_t*>(storageBuffers_[5].mapped);
    pathLengths_ = static_cast<uint32_t*>(storageBuffers_[6].mapped);
    pathCosts_ = static_cast<float*>(storageBuffers_[7].mapped);
    reachedGoal_ = static_cast<uint32_t*>(storageBuffers_[8].mapped);
    interactionCurrent_ = static_cast<float*>(storageBuffers_[9].mapped);
    interactionTotal_ = static_cast<float*>(storageBuffers_[10].mapped);
    interactionHistory_ = static_cast<float*>(storageBuffers_[11].mapped);
    control_ = static_cast<GpuControl*>(storageBuffers_[12].mapped);
    bestPaths_ = static_cast<uint32_t*>(storageBuffers_[13].mapped);
    goalBest_ = static_cast<GpuGoalBest*>(storageBuffers_[2].mapped);
    antStates_ = static_cast<uint32_t*>(storageBuffers_[17].mapped);
    pathStarts_ = static_cast<uint32_t*>(storageBuffers_[18].mapped);
    *control_ = GpuControl{};
    control_->width = scenario_.width;
    control_->height = scenario_.height;
    control_->antCount = antCount_;
    control_->maxPath = maxPathForGrid();
    control_->maxIterations = iterationLimit_;
    control_->startNode = scenario_.source;
    control_->tileWidth = static_cast<uint32_t>(tileWidth);
    control_->tileHeight = static_cast<uint32_t>(tileHeight);
    control_->seed = scenario_.seed;
    control_->guidedMode = guidedMode_ ? 1U : 0U;
    control_->visitedCapacity = static_cast<uint32_t>(visitedCapacity);
    control_->odorRadius = odorRadiusForGrid();
    control_->odorStrength = 8.0F;
    control_->odorEnabled = odorEnabled_ ? 1U : 0U;
    uploadEditableGrid();
}

void VulkanApp::createDescriptors() {
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(storageBuffers_.size())};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = 1U;
    pool.poolSizeCount = 1U;
    pool.pPoolSizes = &size;
    throwIfFailed(vkCreateDescriptorPool(device_, &pool, nullptr, &descriptorPool_), "vkCreateDescriptorPool");
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate.descriptorPool = descriptorPool_;
    allocate.descriptorSetCount = 1U;
    allocate.pSetLayouts = &computeDescriptorLayout_;
    throwIfFailed(vkAllocateDescriptorSets(device_, &allocate, &computeDescriptorSet_), "vkAllocateDescriptorSets");
    std::array<VkDescriptorBufferInfo, 19> infos{};
    std::array<VkWriteDescriptorSet, 19> writes{};
    for (uint32_t index = 0U; index < storageBuffers_.size(); ++index) {
        infos[index] = {storageBuffers_[index].handle, 0U, storageBuffers_[index].size};
        writes[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[index].dstSet = computeDescriptorSet_;
        writes[index].dstBinding = index;
        writes[index].descriptorCount = 1U;
        writes[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[index].pBufferInfo = &infos[index];
    }
    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0U, nullptr);
}

void VulkanApp::createSyncObjects() {
    VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    throwIfFailed(vkCreateSemaphore(device_, &semaphore, nullptr, &imageAvailable_), "vkCreateSemaphore");
    throwIfFailed(vkCreateSemaphore(device_, &semaphore, nullptr, &renderFinished_), "vkCreateSemaphore");
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    throwIfFailed(vkCreateFence(device_, &fence, nullptr, &inFlightFence_), "vkCreateFence");
}

void VulkanApp::uploadEditableGrid() {
    if (control_ == nullptr) return;
    if (scenario_.obstacles.size() > kMaxObstacles || scenario_.goals.size() > kMaxGoals) {
        throw std::runtime_error("Sparse grid capacity exceeded.");
    }
    if (!scenario_.obstacles.empty()) {
        std::memcpy(storageBuffers_[0].mapped, scenario_.obstacles.data(),
                    scenario_.obstacles.size() * sizeof(uint32_t));
    }
    std::memcpy(storageBuffers_[1].mapped, scenario_.goals.data(),
                scenario_.goals.size() * sizeof(uint32_t));
    control_->obstacleCount = static_cast<uint32_t>(scenario_.obstacles.size());
    control_->goalCount = static_cast<uint32_t>(scenario_.goals.size());
    control_->startNode = scenario_.source;
}

void VulkanApp::recalculateOptimalCosts() {
    optimalCosts_.clear();
    optimalCosts_.reserve(scenario_.goals.size());
    optimalCostTotal_ = 0U;
    for (const uint32_t goal : scenario_.goals) {
        const uint32_t cost = optimalGridCost(scenario_, goal);
        optimalCosts_.push_back(cost);
        if (cost == 0U) {
            optimalCostTotal_ = 0U;
            status_ = "HAY UN DESTINO SIN CONEXION";
            return;
        }
        optimalCostTotal_ += cost;
    }
}

uint32_t VulkanApp::maxPathForGrid() const {
    const uint64_t perimeter = static_cast<uint64_t>(scenario_.width) + scenario_.height;
    const uint64_t requested = guidedMode_ ? 4ULL * perimeter + 1024ULL
                                           : maxPathCapacityForGrid();
    return static_cast<uint32_t>(std::clamp<uint64_t>(requested, 4096ULL, kMaxPathCapacity));
}

uint32_t VulkanApp::maxPathCapacityForGrid() const {
    const uint64_t requested = static_cast<uint64_t>(scenario_.width) * scenario_.height;
    return static_cast<uint32_t>(std::clamp<uint64_t>(requested, 4096ULL, kMaxPathCapacity));
}

uint32_t VulkanApp::visitedCapacityForGrid() const {
    const uint64_t nodeCount = static_cast<uint64_t>(scenario_.width) * scenario_.height;
    const uint64_t maximumUniqueNodes = std::min<uint64_t>(maxPathCapacityForGrid(), nodeCount);
    const uint64_t requested = std::max<uint64_t>(2ULL, maximumUniqueNodes * 2ULL);
    uint32_t capacity = 1U;
    while (static_cast<uint64_t>(capacity) < requested) capacity <<= 1U;
    return capacity;
}

float VulkanApp::odorRadiusForGrid() const {
    // Keep smell local and independent of the benchmark dimensions. A target
    // hundreds of cells away in a 1000x1000 grid must be undetectable.
    return 64.0F;
}

float VulkanApp::odorConcentrationAt(const uint32_t node) const {
    if (!odorEnabled_ || scenario_.goals.empty()) return 0.0F;
    const uint32_t x = node % scenario_.width;
    const uint32_t y = node / scenario_.width;
    const float radius = odorRadiusForGrid();
    float maximum = 0.0F;
    for (const uint32_t goal : scenario_.goals) {
        const uint32_t goalX = goal % scenario_.width;
        const uint32_t goalY = goal / scenario_.width;
        const uint32_t distance = (x > goalX ? x - goalX : goalX - x)
                                + (y > goalY ? y - goalY : goalY - y);
        if (static_cast<float>(distance) >= radius) continue;
        const float remaining = 1.0F - static_cast<float>(distance) / radius;
        maximum = std::max(maximum, remaining * remaining);
    }
    return maximum;
}

uint32_t VulkanApp::desiredAntCountForGrid() const {
    const uint32_t longestSide = std::max(scenario_.width, scenario_.height);
    if (longestSide <= 32U) return kBaseAntCount;
    constexpr double minimumRoot = 5.656854249492381;
    constexpr double maximumRoot = 100.0;
    const double normalized = std::clamp(
        (std::sqrt(static_cast<double>(longestSide)) - minimumRoot) / (maximumRoot - minimumRoot),
        0.0, 1.0);
    const double scaled = static_cast<double>(kBaseAntCount)
        + normalized * static_cast<double>(kMaxAntCount - kBaseAntCount);
    return std::clamp(static_cast<uint32_t>(std::lround(scaled)), kBaseAntCount, kMaxAntCount);
}

std::array<float, 2> VulkanApp::gridPoint(const uint32_t node) const {
    const float cameraHeight = cameraWidth_ * 455.0F / 570.0F;
    const float minX = cameraCenterX_ - cameraWidth_ * 0.5F;
    const float minY = cameraCenterY_ - cameraHeight * 0.5F;
    const float x = static_cast<float>(node % scenario_.width) + 0.5F;
    const float y = static_cast<float>(node / scenario_.width) + 0.5F;
    return {{34.0F + (x - minX) / cameraWidth_ * 570.0F,
             24.0F + (y - minY) / cameraHeight * 455.0F}};
}

std::optional<uint32_t> VulkanApp::gridNodeAt(const float x, const float y) const {
    if (x < 34.0F || x >= 604.0F || y < 24.0F || y >= 479.0F) return std::nullopt;
    const float cameraHeight = cameraWidth_ * 455.0F / 570.0F;
    const float gridX = cameraCenterX_ - cameraWidth_ * 0.5F + (x - 34.0F) / 570.0F * cameraWidth_;
    const float gridY = cameraCenterY_ - cameraHeight * 0.5F + (y - 24.0F) / 455.0F * cameraHeight;
    if (gridX < 0.0F || gridY < 0.0F
        || gridX >= static_cast<float>(scenario_.width) || gridY >= static_cast<float>(scenario_.height)) {
        return std::nullopt;
    }
    return static_cast<uint32_t>(gridY) * scenario_.width + static_cast<uint32_t>(gridX);
}

void VulkanApp::applyZoom(const float amount, const float cursorX, const float cursorY) {
    const float oldHeight = cameraWidth_ * 455.0F / 570.0F;
    const float normalizedX = (cursorX - 34.0F) / 570.0F;
    const float normalizedY = (cursorY - 24.0F) / 455.0F;
    const float anchorX = cameraCenterX_ - cameraWidth_ * 0.5F + normalizedX * cameraWidth_;
    const float anchorY = cameraCenterY_ - oldHeight * 0.5F + normalizedY * oldHeight;
    const float fullWidth = std::max(static_cast<float>(scenario_.width),
                                     static_cast<float>(scenario_.height) * 570.0F / 455.0F);
    const float newWidth = std::clamp(cameraWidth_ * std::pow(0.70F, amount), 8.0F, fullWidth);
    const float newHeight = newWidth * 455.0F / 570.0F;
    cameraCenterX_ = anchorX - (normalizedX - 0.5F) * newWidth;
    cameraCenterY_ = anchorY - (normalizedY - 0.5F) * newHeight;
    if (newWidth >= scenario_.width) cameraCenterX_ = static_cast<float>(scenario_.width) * 0.5F;
    else cameraCenterX_ = std::clamp(cameraCenterX_, newWidth * 0.5F,
                                     static_cast<float>(scenario_.width) - newWidth * 0.5F);
    if (newHeight >= scenario_.height) cameraCenterY_ = static_cast<float>(scenario_.height) * 0.5F;
    else cameraCenterY_ = std::clamp(cameraCenterY_, newHeight * 0.5F,
                                     static_cast<float>(scenario_.height) - newHeight * 0.5F);
    cameraWidth_ = newWidth;
}

void VulkanApp::waitForFrame() {
    throwIfFailed(vkWaitForFences(device_, 1U, &inFlightFence_, VK_TRUE, UINT64_MAX), "vkWaitForFences");
}

void VulkanApp::drawFrame() {
    uint32_t imageIndex = 0U;
    const VkResult acquire = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageAvailable_, VK_NULL_HANDLE,
                                                    &imageIndex);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) throwIfFailed(acquire, "vkAcquireNextImageKHR");
    uint32_t iterations = 0U;
    if (!pendingReset_ && control_->iteration < iterationLimit_) {
        if (running_ && autoVerify_) iterations = std::min(10U, iterationLimit_ - control_->iteration);
        else if (running_ && (!hasAntPlayback_ || antPlaybackProgress_ >= 1.0F)) iterations = 1U;
        else if (pendingStep_ && (!hasAntPlayback_ || antPlaybackProgress_ >= 1.0F)) iterations = 1U;
    }
    const bool reset = pendingReset_;
    pendingReset_ = false;
    if (reset || iterations > 0U) pendingStep_ = false;
    recordCommandBuffer(imageIndex, iterations, reset);
    throwIfFailed(vkResetFences(device_, 1U, &inFlightFence_), "vkResetFences");
    constexpr VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1U;
    submit.pWaitSemaphores = &imageAvailable_;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1U;
    submit.pCommandBuffers = &commandBuffer_;
    submit.signalSemaphoreCount = 1U;
    submit.pSignalSemaphores = &renderFinished_;
    throwIfFailed(vkQueueSubmit(graphicsQueue_, 1U, &submit, inFlightFence_), "vkQueueSubmit");
    lastSubmissionHadCompute_ = reset || iterations > 0U;
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1U;
    present.pWaitSemaphores = &renderFinished_;
    present.swapchainCount = 1U;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &imageIndex;
    const VkResult result = vkQueuePresentKHR(presentQueue_, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        framebufferResized_ = false;
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        throwIfFailed(result, "vkQueuePresentKHR");
    }
}

void VulkanApp::recordCommandBuffer(const uint32_t imageIndex, const uint32_t iterations,
                                    const bool resetFirst) {
    throwIfFailed(vkResetCommandBuffer(commandBuffer_, 0U), "vkResetCommandBuffer");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    throwIfFailed(vkBeginCommandBuffer(commandBuffer_, &begin), "vkBeginCommandBuffer");
    vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout_, 0U, 1U,
                            &computeDescriptorSet_, 0U, nullptr);
    const uint32_t matrixCount = antCount_ * antCount_;
    const uint32_t pheromoneSlots = control_->tileWidth * control_->tileHeight * 4U;
    const uint32_t contributionCount = pheromoneSlots * antCount_;
    if (resetFirst) {
        vkCmdFillBuffer(commandBuffer_, storageBuffers_[14].handle, 0U,
                        storageBuffers_[14].size, 0xffffffffU);
        VkMemoryBarrier transferBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        transferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        transferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0U,
                             1U, &transferBarrier, 0U, nullptr, 0U, nullptr);
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, initPipeline_);
        vkCmdDispatch(commandBuffer_, (std::max({matrixCount, contributionCount, control_->goalCount}) + 63U) / 64U,
                      1U, 1U);
        computeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
    }
    for (uint32_t iteration = 0U; iteration < iterations; ++iteration) {
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, beginPipeline_);
        vkCmdDispatch(commandBuffer_, (matrixCount + 63U) / 64U, 1U, 1U);
        computeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, constructPipeline_);
        vkCmdDispatch(commandBuffer_, (antCount_ + 31U) / 32U, 1U, 1U);
        computeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, accumulatePipeline_);
        vkCmdDispatch(commandBuffer_, (matrixCount + 63U) / 64U, 1U, 1U);
        computeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
        vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, advancePipeline_);
        vkCmdDispatch(commandBuffer_, 1U, 1U, 1U);
        computeBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_HOST_READ_BIT);
    }
    VkClearValue clear{{{kClear[0], kClear[1], kClear[2], kClear[3]}}};
    VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render.renderPass = renderPass_;
    render.framebuffer = framebuffers_.at(imageIndex);
    render.renderArea.extent = swapchainExtent_;
    render.clearValueCount = 1U;
    render.pClearValues = &clear;
    vkCmdBeginRenderPass(commandBuffer_, &render, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
    const VkDeviceSize offset = 0U;
    vkCmdBindVertexBuffers(commandBuffer_, 0U, 1U, &vertexBuffer_.handle, &offset);
    vkCmdDraw(commandBuffer_, static_cast<uint32_t>(vertices_.size()), 1U, 0U, 0U);
    vkCmdEndRenderPass(commandBuffer_);
    throwIfFailed(vkEndCommandBuffer(commandBuffer_), "vkEndCommandBuffer");
}

void VulkanApp::computeBarrier(const VkAccessFlags sourceAccess, const VkAccessFlags destinationAccess) {
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = sourceAccess;
    barrier.dstAccessMask = destinationAccess;
    vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
                         0U, 1U, &barrier, 0U, nullptr, 0U, nullptr);
}

void VulkanApp::processInput() {
    if (keyPressedOnce(GLFW_KEY_SPACE)) {
        if (optimalCostTotal_ == 0U) {
            running_ = false;
            status_ = "HAY UN DESTINO SIN CONEXION";
        } else {
            running_ = !running_;
            status_ = running_ ? "EJECUTANDO ACO" : "PAUSA";
        }
    }
    if (keyPressedOnce(GLFW_KEY_N)) {
        running_ = false;
        pendingStep_ = optimalCostTotal_ != 0U;
        status_ = pendingStep_ ? "UNA ITERACION" : "HAY UN DESTINO SIN CONEXION";
    }
    if (keyPressedOnce(GLFW_KEY_R)) resetSimulation();
    if (keyPressedOnce(GLFW_KEY_M)) setGuidedMode(!guidedMode_);
    if (keyPressedOnce(GLFW_KEY_L)) setOdorEnabled(!odorEnabled_);
    if (keyPressedOnce(GLFW_KEY_LEFT_BRACKET)) changeIterationLimit(-100);
    if (keyPressedOnce(GLFW_KEY_RIGHT_BRACKET)) changeIterationLimit(100);
    if (keyPressedOnce(GLFW_KEY_1)) view_ = View::Problem;
    if (keyPressedOnce(GLFW_KEY_2)) view_ = View::Interaction;
    if (keyPressedOnce(GLFW_KEY_3)) view_ = View::Metrics;
    if (keyPressedOnce(GLFW_KEY_S)) {
        editTool_ = EditTool::Start;
        view_ = View::Problem;
        status_ = "SELECCIONA ORIGEN";
    }
    if (keyPressedOnce(GLFW_KEY_G)) {
        editTool_ = EditTool::Goal;
        view_ = View::Problem;
        status_ = "AGREGA O QUITA DESTINOS";
    }
    if (keyPressedOnce(GLFW_KEY_O)) {
        editTool_ = EditTool::Obstacle;
        view_ = View::Problem;
        status_ = "DIBUJA OBSTACULOS";
    }
    if (keyPressedOnce(GLFW_KEY_LEFT) && !history_.empty()) {
        followLatest_ = false;
        if (selectedIteration_ > 0U) --selectedIteration_;
    }
    if (keyPressedOnce(GLFW_KEY_RIGHT) && !history_.empty()) {
        if (selectedIteration_ + 1U < history_.size()) ++selectedIteration_;
        followLatest_ = selectedIteration_ + 1U == history_.size();
    }
    double rawX = 0.0;
    double rawY = 0.0;
    glfwGetCursorPos(window_, &rawX, &rawY);
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window_, &windowWidth, &windowHeight);
    const float x = windowWidth > 0 ? static_cast<float>(rawX) * kLogicalWidth / static_cast<float>(windowWidth) : 0.0F;
    const float y = windowHeight > 0 ? static_cast<float>(rawY) * kLogicalHeight / static_cast<float>(windowHeight) : 0.0F;
    if (pendingScroll_ != 0.0F) {
        if (view_ == View::Problem && x >= 34.0F && x < 604.0F && y >= 24.0F && y < 479.0F) {
            applyZoom(pendingScroll_, x, y);
        }
        pendingScroll_ = 0.0F;
    }
    const bool rightDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (rightDown && rightMouseWasDown_ && view_ == View::Problem && windowWidth > 0 && windowHeight > 0) {
        const float dx = static_cast<float>(rawX - lastRightMouseX_) * kLogicalWidth / static_cast<float>(windowWidth);
        const float dy = static_cast<float>(rawY - lastRightMouseY_) * kLogicalHeight / static_cast<float>(windowHeight);
        const float cameraHeight = cameraWidth_ * 455.0F / 570.0F;
        cameraCenterX_ -= dx / 570.0F * cameraWidth_;
        cameraCenterY_ -= dy / 455.0F * cameraHeight;
        if (cameraWidth_ >= scenario_.width) cameraCenterX_ = static_cast<float>(scenario_.width) * 0.5F;
        else cameraCenterX_ = std::clamp(cameraCenterX_, cameraWidth_ * 0.5F,
                                         static_cast<float>(scenario_.width) - cameraWidth_ * 0.5F);
        if (cameraHeight >= scenario_.height) cameraCenterY_ = static_cast<float>(scenario_.height) * 0.5F;
        else cameraCenterY_ = std::clamp(cameraCenterY_, cameraHeight * 0.5F,
                                         static_cast<float>(scenario_.height) - cameraHeight * 0.5F);
    }
    lastRightMouseX_ = rawX;
    lastRightMouseY_ = rawY;
    rightMouseWasDown_ = rightDown;

    const bool mouseDown = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseDown && (!leftMouseWasDown_ || editTool_ == EditTool::Obstacle)) {
        if (editTool_ == EditTool::Obstacle && view_ == View::Problem
            && x >= 34.0F && x < 604.0F && y >= 24.0F && y < 479.0F) {
            handleGridEdit(x, y, leftMouseWasDown_);
        } else if (!leftMouseWasDown_) {
            handleClick(x, y);
        }
    }
    if (!mouseDown) lastPaintedNode_ = std::numeric_limits<uint32_t>::max();
    leftMouseWasDown_ = mouseDown;
}

void VulkanApp::handleClick(const float x, const float y) {
    if (view_ == View::Problem && x >= 34.0F && x < 604.0F && y >= 24.0F && y < 479.0F) {
        handleGridEdit(x, y, false);
        return;
    }
    if (contains({670, 58, 815, 96}, x, y)) {
        if (optimalCostTotal_ == 0U) {
            running_ = false;
            status_ = "HAY UN DESTINO SIN CONEXION";
        } else {
            running_ = !running_;
            status_ = running_ ? "EJECUTANDO ACO" : "PAUSA";
        }
        return;
    }
    if (contains({835, 58, 980, 96}, x, y)) {
        running_ = false;
        pendingStep_ = optimalCostTotal_ != 0U;
        if (!pendingStep_) status_ = "HAY UN DESTINO SIN CONEXION";
        return;
    }
    if (contains({670, 108, 820, 146}, x, y)) {
        resetSimulation();
        return;
    }
    if (contains({828, 108, 900, 146}, x, y)) {
        changeIterationLimit(-100);
        return;
    }
    if (contains({908, 108, 980, 146}, x, y)) {
        changeIterationLimit(100);
        return;
    }
    if (contains({670, 154, 768, 190}, x, y)) {
        editTool_ = EditTool::Start;
        view_ = View::Problem;
        status_ = "SELECCIONA ORIGEN";
        return;
    }
    if (contains({776, 154, 874, 190}, x, y)) {
        editTool_ = EditTool::Goal;
        view_ = View::Problem;
        status_ = "AGREGA O QUITA DESTINOS";
        return;
    }
    if (contains({882, 154, 980, 190}, x, y)) {
        editTool_ = EditTool::Obstacle;
        view_ = View::Problem;
        status_ = "DIBUJA OBSTACULOS";
        return;
    }
    const std::array<Rect, 3> views{{{670, 202, 768, 238}, {776, 202, 874, 238}, {882, 202, 980, 238}}};
    for (uint32_t index = 0U; index < views.size(); ++index) {
        if (contains(views[index], x, y)) view_ = static_cast<View>(index);
    }
    if (contains({670, 337, 768, 373}, x, y)) {
        setGuidedMode(true);
        return;
    }
    if (contains({776, 337, 874, 373}, x, y)) {
        setGuidedMode(false);
        return;
    }
    if (contains({882, 337, 980, 373}, x, y)) {
        setOdorEnabled(!odorEnabled_);
        return;
    }
    const std::array<Rect, 3> speeds{{{670, 536, 768, 572}, {776, 536, 874, 572}, {882, 536, 980, 572}}};
    const float values[] = {1.0F, 5.0F, 20.0F};
    for (uint32_t index = 0U; index < speeds.size(); ++index) {
        if (contains(speeds[index], x, y)) {
            playbackSpeed_ = values[index];
            status_ = "VELOCIDAD X" + std::to_string(static_cast<uint32_t>(playbackSpeed_));
        }
    }
    if (contains({670, 585, 815, 621}, x, y) && !history_.empty()) {
        followLatest_ = false;
        if (selectedIteration_ > 0U) --selectedIteration_;
    }
    if (contains({835, 585, 980, 621}, x, y) && !history_.empty()) {
        if (selectedIteration_ + 1U < history_.size()) ++selectedIteration_;
        followLatest_ = selectedIteration_ + 1U == history_.size();
    }
}

void VulkanApp::handleGridEdit(const float x, const float y, const bool dragging) {
    const std::optional<uint32_t> selected = gridNodeAt(x, y);
    if (!selected.has_value() || (dragging && *selected == lastPaintedNode_)) return;
    const uint32_t node = *selected;
    lastPaintedNode_ = node;
    if (editTool_ == EditTool::Start) {
        if (!scenario_.traversable(node) || scenario_.isGoal(node)) {
            status_ = "ORIGEN INVALIDO";
            return;
        }
        scenario_.source = node;
        status_ = "ORIGEN ACTUALIZADO";
    } else if (editTool_ == EditTool::Goal) {
        if (!scenario_.traversable(node) || node == scenario_.source) {
            status_ = "DESTINO INVALIDO";
            return;
        }
        const auto goal = std::lower_bound(scenario_.goals.begin(), scenario_.goals.end(), node);
        if (goal != scenario_.goals.end() && *goal == node) {
            if (scenario_.goals.size() == 1U) {
                status_ = "SE REQUIERE UN DESTINO";
                return;
            }
            scenario_.goals.erase(goal);
            status_ = "DESTINO ELIMINADO";
        } else {
            if (scenario_.goals.size() >= kMaxGoals) {
                status_ = "MAXIMO 30 DESTINOS";
                return;
            }
            scenario_.goals.insert(goal, node);
            status_ = "DESTINO AGREGADO";
        }
    } else {
        if (node == scenario_.source || scenario_.isGoal(node)) {
            status_ = "NO BLOQUEES UN EXTREMO";
            return;
        }
        const auto obstacle = std::lower_bound(scenario_.obstacles.begin(), scenario_.obstacles.end(), node);
        if (obstacle != scenario_.obstacles.end() && *obstacle == node) {
            scenario_.obstacles.erase(obstacle);
            status_ = "OBSTACULO ELIMINADO";
        } else {
            if (scenario_.obstacles.size() >= kMaxObstacles) {
                status_ = "LIMITE DE OBSTACULOS";
                return;
            }
            scenario_.obstacles.insert(obstacle, node);
            status_ = "OBSTACULO AGREGADO";
        }
    }
    const std::string editStatus = status_;
    recalculateOptimalCosts();
    uploadEditableGrid();
    resetSimulation();
    status_ = optimalCostTotal_ == 0U ? "HAY UN DESTINO SIN CONEXION" : editStatus;
}

void VulkanApp::changeIterationLimit(const int32_t delta) {
    const int64_t proposed = static_cast<int64_t>(iterationLimit_) + delta;
    const uint32_t updated = static_cast<uint32_t>(
        std::clamp<int64_t>(proposed, 1, static_cast<int64_t>(kIterationCapacity)));
    if (updated == iterationLimit_) {
        status_ = updated == 1U ? "MINIMO 1 ITERACION" : "MAXIMO 10000 ITERACIONES";
        return;
    }
    iterationLimit_ = updated;
    resetSimulation();
    status_ = "ITERACIONES " + std::to_string(iterationLimit_);
}

void VulkanApp::setGuidedMode(const bool guided) {
    if (guidedMode_ == guided) return;
    guidedMode_ = guided;
    resetSimulation();
    status_ = guidedMode_ ? "MODO GUIADO" : "SIN GUIA: EXPLORACION LIBRE";
}

void VulkanApp::setOdorEnabled(const bool enabled) {
    if (odorEnabled_ == enabled) return;
    odorEnabled_ = enabled;
    resetSimulation();
    status_ = odorEnabled_ ? "OLFATO ACTIVADO: RADIO 64" : "OLFATO DESACTIVADO";
}

float VulkanApp::antPlaybackDurationSeconds() const {
    const bool firstIteration = antPlaybackGeneration_ <= 1U;
    if (guidedMode_) return firstIteration ? 8.0F : 2.5F;
    constexpr float unguidedStepsPerSecond = 240.0F;
    const uint32_t steps = antPlaybackMaxSteps_ > 0U ? antPlaybackMaxSteps_ : maxPathForGrid();
    const float routeDuration = static_cast<float>(steps) / unguidedStepsPerSecond;
    return std::max(firstIteration ? 10.0F : 3.0F, routeDuration);
}

void VulkanApp::updateAntAnimation() {
    const double now = glfwGetTime();
    const float elapsed = static_cast<float>(std::clamp(now - lastAnimationTime_, 0.0, 0.10));
    lastAnimationTime_ = now;
    if (!hasAntPlayback_ || antPlaybackProgress_ >= 1.0F) return;
    const float baseDurationSeconds = antPlaybackDurationSeconds();
    const float previousProgress = antPlaybackProgress_;
    antPlaybackProgress_ = std::min(1.0F,
        antPlaybackProgress_ + elapsed * playbackSpeed_ / baseDurationSeconds);
    if (previousProgress < 1.0F && antPlaybackProgress_ >= 1.0F && control_ != nullptr) {
        if (!antPlaybackCompletesGeneration_) {
            status_ = "T" + std::to_string(antPlaybackGeneration_) + " SIGUE BUSCANDO COMIDA";
        } else {
            status_ = antPlaybackGeneration_ >= iterationLimit_
                ? std::to_string(iterationLimit_) + " GENERACIONES COMPLETAS"
                : antPlaybackGeneration_ == 1U ? "T1 PRIMERA RUTA COMPLETA"
                : "GENERACION " + std::to_string(antPlaybackGeneration_) + " COMPLETA";
        }
    }
}

void VulkanApp::captureAntRoutes(const uint32_t iteration) {
    antDisplayRoutes_.assign(antCount_, {});
    antDisplayRouteStarts_.assign(antCount_, scenario_.source);
    antPlaybackMaxSteps_ = 0U;
    bool anyRoute = false;
    const bool completesGeneration = control_ != nullptr && control_->iteration > iteration;
    for (uint32_t ant = 0U; ant < antCount_; ++ant) {
        antDisplayRouteStarts_[ant] = pathStarts_[ant];
        const uint32_t routeIndex = iteration * antCount_ + ant;
        const uint32_t length = std::min(pathLengths_[routeIndex], control_->maxPath);
        if (length == 0U) {
            if (antStates_[ant * 4U + 2U] == 1U) {
                const uint32_t current = antStates_[ant * 4U];
                antDisplayRouteStarts_[ant] = current;
                antDisplayRoutes_[ant].push_back(current);
                antPlaybackMaxSteps_ = std::max(antPlaybackMaxSteps_, 1U);
                anyRoute = true;
            }
            continue;
        }
        const uint32_t* route = pathNodes_ + static_cast<std::size_t>(ant) * control_->maxPath;
        antDisplayRoutes_[ant].assign(route, route + length);
        antPlaybackMaxSteps_ = std::max(antPlaybackMaxSteps_, length);
        anyRoute = true;
    }
    if (iteration == 0U && completesGeneration) {
        for (uint32_t goalIndex = 0U; goalIndex < bestRoutes_.size(); ++goalIndex) {
            const uint32_t ant = goalBest_[goalIndex].ant;
            if (ant >= antCount_ || bestRoutes_[goalIndex].empty()) continue;
            antDisplayRouteStarts_[ant] = scenario_.source;
            antDisplayRoutes_[ant] = bestRoutes_[goalIndex];
            antPlaybackMaxSteps_ = std::max<uint32_t>(
                antPlaybackMaxSteps_, static_cast<uint32_t>(bestRoutes_[goalIndex].size()));
            anyRoute = true;
        }
    }
    hasAntPlayback_ = anyRoute;
    antPlaybackProgress_ = anyRoute ? 0.0F : 1.0F;
    antPlaybackGeneration_ = iteration + 1U;
    antPlaybackCompletesGeneration_ = completesGeneration;
    lastAnimationTime_ = glfwGetTime();
    if (anyRoute) {
        status_ = antPlaybackCompletesGeneration_
            ? antPlaybackGeneration_ == 1U ? "T1 PRIMERA RUTA ENCONTRADA"
                                           : "T" + std::to_string(antPlaybackGeneration_) + " COLONIA COMPLETA"
            : "RECORRIENDO BUSQUEDA CONTINUA T" + std::to_string(antPlaybackGeneration_);
    }
}

void VulkanApp::resetSimulation() {
    running_ = false;
    pendingReset_ = true;
    pendingStep_ = false;
    lastProcessedIteration_ = 0U;
    lastProcessedSearchTick_ = 0U;
    selectedIteration_ = 0U;
    followLatest_ = true;
    bestCost_ = std::numeric_limits<float>::infinity();
    bestFoundAt_ = 0U;
    bestAnt_ = 0U;
    bestRoutes_.assign(scenario_.goals.size(), {});
    bestTotalHistory_.clear();
    antDisplayRoutes_.assign(antCount_, {});
    antDisplayRouteStarts_.assign(antCount_, scenario_.source);
    antPlaybackProgress_ = 1.0F;
    antPlaybackMaxSteps_ = 0U;
    antPlaybackGeneration_ = 0U;
    hasAntPlayback_ = false;
    antPlaybackCompletesGeneration_ = false;
    history_.clear();
    status_ = optimalCostTotal_ == 0U ? "HAY UN DESTINO SIN CONEXION" : "COLONIA REINICIADA";
    if (control_ != nullptr) {
        *control_ = GpuControl{};
        control_->seed = scenario_.seed;
        control_->width = scenario_.width;
        control_->height = scenario_.height;
        control_->antCount = antCount_;
        control_->maxPath = maxPathForGrid();
        control_->maxIterations = iterationLimit_;
        control_->startNode = scenario_.source;
        control_->tileWidth = std::min(scenario_.width, kPheromoneResolution);
        control_->tileHeight = std::min(scenario_.height, kPheromoneResolution);
        control_->guidedMode = guidedMode_ ? 1U : 0U;
        control_->visitedCapacity = visitedCapacityForGrid();
        control_->odorRadius = odorRadiusForGrid();
        control_->odorStrength = 8.0F;
        control_->odorEnabled = odorEnabled_ ? 1U : 0U;
        uploadEditableGrid();
    }
}

void VulkanApp::consumeGpuResults() {
    if (!lastSubmissionHadCompute_) return;
    lastSubmissionHadCompute_ = false;
    const uint32_t completed = std::min(control_->iteration, iterationLimit_);
    const bool hasNewIteration = completed > lastProcessedIteration_;
    const bool hasNewSearchBlock = completed == lastProcessedIteration_
        && control_->generationSearchTicks > lastProcessedSearchTick_;
    if (autoVerify_ && hasNewSearchBlock
        && control_->generationSearchTicks / 256U > lastProcessedSearchTick_ / 256U) {
        uint32_t searching = 0U;
        uint32_t reached = 0U;
        uint32_t exhausted = 0U;
        uint32_t maximumAttempt = 0U;
        for (uint32_t ant = 0U; ant < antCount_; ++ant) {
            const uint32_t status = antStates_[ant * 4U + 2U];
            searching += status == 0U ? 1U : 0U;
            reached += status == 1U || status == 2U ? 1U : 0U;
            exhausted += status == 3U ? 1U : 0U;
            maximumAttempt = std::max(maximumAttempt, antStates_[ant * 4U + 3U] >> 18U);
        }
        std::cout << "GPU search progress: T=" << completed + 1U
                  << ", blocks=" << control_->generationSearchTicks
                  << ", searching=" << searching << ", reached=" << reached
                  << ", exhausted=" << exhausted << ", maxAttempt=" << maximumAttempt << '\n';
    }
    for (uint32_t iteration = lastProcessedIteration_; iteration < completed; ++iteration) processIteration(iteration);
    if (!autoVerify_) {
        if (hasNewIteration) captureAntRoutes(completed - 1U);
        else if (hasNewSearchBlock) captureAntRoutes(completed);
    }
    lastProcessedIteration_ = completed;
    lastProcessedSearchTick_ = control_->generationSearchTicks;
    if (followLatest_ && !history_.empty()) selectedIteration_ = static_cast<uint32_t>(history_.size() - 1U);
    const bool playingRoutes = !autoVerify_ && hasAntPlayback_ && antPlaybackProgress_ < 1.0F;
    if (completed >= iterationLimit_) running_ = false;
    if (playingRoutes) {
        status_ = antPlaybackCompletesGeneration_
            ? antPlaybackGeneration_ == 1U ? "T1 PRIMERA RUTA ENCONTRADA"
                                           : "T" + std::to_string(antPlaybackGeneration_) + " COLONIA COMPLETA"
            : "RECORRIENDO BUSQUEDA CONTINUA T" + std::to_string(antPlaybackGeneration_);
    } else if (completed >= iterationLimit_) {
        status_ = std::to_string(iterationLimit_) + " GENERACIONES COMPLETAS";
    } else if (!history_.empty()) {
        status_ = "GPU VALIDADA";
    }
    if (autoVerify_ && completed >= iterationLimit_) {
        bool diagonalZero = true;
        for (uint32_t ant = 0U; ant < antCount_; ++ant) {
            diagonalZero = diagonalZero && std::abs(interactionTotal_[ant * antCount_ + ant]) < 1.0e-6F;
        }
        const bool allGoalsReached = bestRoutes_.size() == scenario_.goals.size()
            && std::all_of(bestRoutes_.begin(), bestRoutes_.end(), [](const auto& route) { return !route.empty(); });
        const bool valid = std::isfinite(bestCost_) && allGoalsReached && diagonalZero;
        std::cout << "GPU verification " << (valid ? "passed" : "failed")
                  << ": grid=" << scenario_.width << 'x' << scenario_.height
                  << ", iterations=" << completed << ", goals=" << scenario_.goals.size()
                  << ", T1Blocks=" << control_->t1SearchTicks + 1U
                  << ", bestTotal=" << bestCost_
                  << ", firstTBest=" << (bestTotalHistory_.empty() ? bestCost_ : bestTotalHistory_.front())
                  << ", dijkstraTotal=" << optimalCostTotal_
                  << ", gap=" << (bestCost_ / static_cast<float>(optimalCostTotal_) - 1.0F) * 100.0F << '%'
                  << ", diagonalZero=" << diagonalZero << '\n';
        if (!valid) throw std::runtime_error("Network ACO GPU invariant verification failed.");
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

void VulkanApp::processIteration(const uint32_t iteration) {
    IterationStats stats;
    stats.iteration = iteration + 1U;
    stats.bestCost = std::numeric_limits<float>::infinity();
    float sum = 0.0F;
    for (uint32_t ant = 0U; ant < antCount_; ++ant) {
        const uint32_t routeIndex = iteration * antCount_ + ant;
        if (reachedGoal_[routeIndex] == 0U) continue;
        ++stats.successfulAnts;
        sum += pathCosts_[routeIndex];
        if (pathCosts_[routeIndex] < stats.bestCost) {
            stats.bestCost = pathCosts_[routeIndex];
            stats.bestAnt = ant;
        }
    }
    if (bestRoutes_.size() != scenario_.goals.size()) bestRoutes_.assign(scenario_.goals.size(), {});
    bool allGoalsReached = true;
    float totalBestCost = 0.0F;
    uint32_t latestDiscovery = 0U;
    for (uint32_t goalIndex = 0U; goalIndex < scenario_.goals.size(); ++goalIndex) {
        const GpuGoalBest& metadata = goalBest_[goalIndex];
        if (!std::isfinite(metadata.cost) || metadata.length == 0U || metadata.length > control_->maxPath) {
            allGoalsReached = false;
            continue;
        }
        const uint32_t* path = bestPaths_ + static_cast<std::size_t>(goalIndex) * control_->maxPath;
        uint32_t previous = scenario_.source;
        std::unordered_set<uint32_t> visited;
        visited.reserve(static_cast<std::size_t>(metadata.length) + 1U);
        visited.insert(previous);
        for (uint32_t step = 0U; step < metadata.length; ++step) {
            const uint32_t next = path[step];
            const uint32_t previousX = previous % scenario_.width;
            const uint32_t previousY = previous / scenario_.width;
            const uint32_t nextX = next % scenario_.width;
            const uint32_t nextY = next / scenario_.width;
            const uint32_t distance = (previousX > nextX ? previousX - nextX : nextX - previousX)
                                    + (previousY > nextY ? previousY - nextY : nextY - previousY);
            if (next >= scenario_.nodeCount() || !scenario_.traversable(next) || distance != 1U) {
                throw std::runtime_error("GPU produced a non-contiguous implicit-grid route.");
            }
            if (!visited.insert(next).second) {
                throw std::runtime_error("GPU route revisited a cell excluded by the tabu memory.");
            }
            previous = next;
        }
        if (previous != scenario_.goals[goalIndex] || std::abs(metadata.cost - static_cast<float>(metadata.length)) > 0.01F) {
            throw std::runtime_error("GPU route does not end at its assigned goal.");
        }
        bestRoutes_[goalIndex].assign(path, path + metadata.length);
        totalBestCost += metadata.cost;
        latestDiscovery = std::max(latestDiscovery, metadata.iteration);
    }
    if (allGoalsReached) {
        bestCost_ = totalBestCost;
        bestFoundAt_ = latestDiscovery;
    }
    bestTotalHistory_.push_back(bestCost_);
    stats.averageCost = stats.successfulAnts > 0U ? sum / static_cast<float>(stats.successfulAnts)
                                                  : std::numeric_limits<float>::infinity();
    const std::size_t matrixSize = static_cast<std::size_t>(antCount_) * antCount_;
    const float* matrix = interactionHistory_ + static_cast<std::size_t>(iteration) * matrixSize;
    stats.interaction.assign(matrix, matrix + matrixSize);
    stats.influence.outStrength.assign(antCount_, 0.0F);
    stats.influence.inStrength.assign(antCount_, 0.0F);
    for (uint32_t source = 0U; source < antCount_; ++source) {
        for (uint32_t target = 0U; target < antCount_; ++target) {
            const float value = stats.interaction[source * antCount_ + target];
            stats.influence.outStrength[source] += value;
            stats.influence.inStrength[target] += value;
        }
    }
    const auto strongest = std::max_element(stats.influence.outStrength.begin(), stats.influence.outStrength.end());
    stats.influence.mostInfluentialAnt = static_cast<uint32_t>(strongest - stats.influence.outStrength.begin());
    stats.influence.maxOutStrength = *strongest;
    const float total = std::accumulate(stats.influence.outStrength.begin(), stats.influence.outStrength.end(), 0.0F);
    if (total > 0.0F) {
        stats.influence.concentration = stats.influence.maxOutStrength / total;
        for (const float strength : stats.influence.outStrength) {
            if (strength <= 0.0F) continue;
            const float probability = strength / total;
            stats.influence.entropy -= probability * std::log(probability);
        }
    }
    history_.push_back(std::move(stats));
}

void VulkanApp::rebuildUi() {
    vertices_.clear();
    appendRect({0, 0, 640, 520}, kClear);
    appendRect({0, 520, 640, 720}, kPanel);
    appendRect({640, 0, 650, 720}, kClear);
    appendRect({650, 0, 1000, 720}, kSidebar);
    appendRect({650, 0, 1000, 5}, kCyan);
    if (view_ == View::Problem) drawProblemGraph();
    else if (view_ == View::Interaction) drawInteractionGraph();
    else drawMetrics();

    appendText("NETWORK ACO", 670, 18, 4.0F, kText);
    appendText("ZELINKA + VULKAN COMPUTE", 670, 47, 1.25F, kCyan);
    const auto button = [&](const Rect rect, const std::string& label, const bool selected = false) {
        appendRect(rect, selected ? kSelected : kButton);
        const float width = static_cast<float>(label.size()) * 10.8F;
        appendText(label, (rect.minX + rect.maxX - width) * 0.5F, rect.minY + 12.0F, 1.8F, kText);
    };
    button({670, 58, 815, 96}, running_ ? "PAUSA" : "INICIAR", running_);
    button({835, 58, 980, 96}, "PASO");
    button({670, 108, 820, 146}, "REINICIAR");
    button({828, 108, 900, 146}, "-100");
    button({908, 108, 980, 146}, "+100");
    button({670, 154, 768, 190}, "INICIO", editTool_ == EditTool::Start);
    button({776, 154, 874, 190}, "DESTINO", editTool_ == EditTool::Goal);
    button({882, 154, 980, 190}, "OBSTACULO", editTool_ == EditTool::Obstacle);
    button({670, 202, 768, 238}, "GRAFO", view_ == View::Problem);
    button({776, 202, 874, 238}, "RED", view_ == View::Interaction);
    button({882, 202, 980, 238}, "METRICAS", view_ == View::Metrics);

    appendText("PARAMETROS", 670, 248, 2.2F, kMuted);
    appendText("GRILLA " + std::to_string(scenario_.width) + "X" + std::to_string(scenario_.height),
               670, 274, 1.6F, kText);
    appendText("FINES " + std::to_string(scenario_.goals.size()) + "  OBST "
               + std::to_string(scenario_.obstacles.size()), 670, 297, 1.6F, kText);
    appendText("HORM " + std::to_string(antCount_) + " TODAS RULETA  ITER "
               + std::to_string(iterationLimit_), 670, 320, 1.25F, kText);
    button({670, 337, 768, 373}, "GUIADO", guidedMode_);
    button({776, 337, 874, 373}, "SIN GUIA", !guidedMode_);
    button({882, 337, 980, 373}, "OLFATO", odorEnabled_);
    appendText("PASOS " + std::to_string(maxPathForGrid())
               + (odorEnabled_ ? "  OLOR R" + std::to_string(static_cast<uint32_t>(odorRadiusForGrid()))
                               : "  OLOR OFF"),
               670, 382, 1.35F, kText);
    appendText("OPTIMO TOTAL " + std::to_string(optimalCostTotal_), 670, 405, 1.6F, kGreen);
    std::string iterationText = "T1 BUSQUEDA CONTINUA";
    if (control_ != nullptr) {
        if (hasAntPlayback_ && antPlaybackProgress_ < 1.0F) {
            iterationText = "T" + std::to_string(antPlaybackGeneration_)
                + (antPlaybackCompletesGeneration_ ? " LLEGANDO A COMIDA" : " BUSQUEDA CONTINUA");
        } else if (control_->iteration >= iterationLimit_) {
            iterationText = "ITERACION " + std::to_string(iterationLimit_) + "/"
                + std::to_string(iterationLimit_);
        } else {
            iterationText = "T" + std::to_string(control_->iteration + 1U) + " BUSQUEDA CONTINUA";
        }
    }
    appendText(iterationText, 670, 433, 2.0F, kCyan);
    if (std::isfinite(bestCost_)) {
        std::ostringstream best;
        best.setf(std::ios::fixed);
        best.precision(2);
        best << "MEJOR COSTO " << bestCost_;
        appendText(best.str(), 670, 462, 1.8F, kGold);
        const float gap = optimalCostTotal_ > 0U
            ? (bestCost_ / static_cast<float>(optimalCostTotal_) - 1.0F) * 100.0F : 0.0F;
        std::ostringstream gapText;
        gapText.setf(std::ios::fixed);
        gapText.precision(1);
        gapText << "BRECHA " << gap << "%  ITER " << bestFoundAt_;
        appendText(gapText.str(), 670, 488, 1.5F, kText);
    }
    const uint32_t visibleSeconds = static_cast<uint32_t>(
        std::ceil(antPlaybackDurationSeconds() / playbackSpeed_));
    appendText("DURACION APROX " + std::to_string(visibleSeconds) + "S", 670, 515, 1.35F, kMuted);
    button({670, 536, 768, 572}, "X1", playbackSpeed_ == 1.0F);
    button({776, 536, 874, 572}, "X5", playbackSpeed_ == 5.0F);
    button({882, 536, 980, 572}, "X20", playbackSpeed_ == 20.0F);
    button({670, 585, 815, 621}, "ANTERIOR");
    button({835, 585, 980, 621}, "SIGUIENTE");
    appendText("S INICIO G FIN O OBSTACULO", 670, 642, 1.25F, kGreen);
    appendText("RUEDA ZOOM  CLICK DER MUEVE", 670, 666, 1.25F, kMuted);
    appendText("M MODO L OLFATO [ ] ITER", 670, 690, 1.25F, kMuted);

    appendText(view_ == View::Problem ? "GRAFO DEL PROBLEMA" :
               view_ == View::Interaction ? "RED DE INTERACCION" : "EVOLUCION DE METRICAS",
               18, 548, 3.0F, kText);
    appendText("ESTADO: " + status_, 18, 582, 2.0F, kCyan);
    if (!history_.empty()) {
        const IterationStats& selected = history_.at(std::min<std::size_t>(selectedIteration_, history_.size() - 1U));
        std::ostringstream summary;
        summary.setf(std::ios::fixed);
        summary.precision(3);
        summary << "T " << selected.iteration << "  EXITO " << selected.successfulAnts << '/' << antCount_
                << "  DOMINANTE ANT " << selected.influence.mostInfluentialAnt;
        appendText(summary.str(), 18, 616, 1.8F, kText);
        std::ostringstream network;
        network.setf(std::ios::fixed);
        network.precision(3);
        network << "CONCENTRACION " << selected.influence.concentration
                << "  ENTROPIA " << selected.influence.entropy;
        appendText(network.str(), 18, 648, 1.8F, kGold);
    } else {
        appendText("DOS GRAFOS SEPARADOS: PROBLEMA E INFLUENCIA", 18, 622, 1.7F, kMuted);
    }
    appendText("INFLUENCIA SE MIDE ANTES DEL NUEVO DEPOSITO", 18, 686, 1.5F, kPurple);
    if (vertices_.size() > kVertexCapacity) throw std::runtime_error("UI vertex capacity exceeded.");
    std::memcpy(vertexBuffer_.mapped, vertices_.data(), vertices_.size() * sizeof(Vertex));
}

void VulkanApp::drawProblemGraph() {
    appendRect({34.0F, 24.0F, 604.0F, 479.0F}, std::array<float, 4>{{0.075F, 0.09F, 0.115F, 1.0F}});
    const float cameraHeight = cameraWidth_ * 455.0F / 570.0F;
    const float minX = cameraCenterX_ - cameraWidth_ * 0.5F;
    const float maxX = cameraCenterX_ + cameraWidth_ * 0.5F;
    const float minY = cameraCenterY_ - cameraHeight * 0.5F;
    const float maxY = cameraCenterY_ + cameraHeight * 0.5F;
    const float cellWidth = 570.0F / cameraWidth_;
    const float cellHeight = 455.0F / cameraHeight;

    if (odorEnabled_ && control_ != nullptr) {
        const uint32_t odorTilesX = std::min(control_->tileWidth, 64U);
        const uint32_t odorTilesY = std::min(control_->tileHeight, 64U);
        for (uint32_t tileY = 0U; tileY < odorTilesY; ++tileY) {
            const float y0 = static_cast<float>(tileY) * scenario_.height / odorTilesY;
            const float y1 = static_cast<float>(tileY + 1U) * scenario_.height / odorTilesY;
            if (y1 < minY || y0 > maxY) continue;
            for (uint32_t tileX = 0U; tileX < odorTilesX; ++tileX) {
                const float x0 = static_cast<float>(tileX) * scenario_.width / odorTilesX;
                const float x1 = static_cast<float>(tileX + 1U) * scenario_.width / odorTilesX;
                if (x1 < minX || x0 > maxX) continue;
                const uint32_t sampleX = std::min(scenario_.width - 1U,
                    static_cast<uint32_t>((x0 + x1) * 0.5F));
                const uint32_t sampleY = std::min(scenario_.height - 1U,
                    static_cast<uint32_t>((y0 + y1) * 0.5F));
                const float concentration = odorConcentrationAt(sampleY * scenario_.width + sampleX);
                if (concentration < 0.0025F) continue;
                appendRect({34.0F + (x0 - minX) / cameraWidth_ * 570.0F,
                            24.0F + (y0 - minY) / cameraHeight * 455.0F,
                            34.0F + (x1 - minX) / cameraWidth_ * 570.0F,
                            24.0F + (y1 - minY) / cameraHeight * 455.0F},
                           std::array<float, 4>{{0.22F + concentration * 0.42F, 0.04F,
                                                0.18F + concentration * 0.38F, 0.28F}});
            }
        }
    }

    if (totalPheromone_ != nullptr && control_ != nullptr) {
        uint32_t maximum = 1U;
        const uint32_t slotCount = control_->tileWidth * control_->tileHeight * 4U;
        for (uint32_t slot = 0U; slot < slotCount; ++slot) maximum = std::max(maximum, totalPheromone_[slot]);
        for (uint32_t tileY = 0U; tileY < control_->tileHeight; ++tileY) {
            const float y0 = static_cast<float>(tileY) * scenario_.height / control_->tileHeight;
            const float y1 = static_cast<float>(tileY + 1U) * scenario_.height / control_->tileHeight;
            if (y1 < minY || y0 > maxY) continue;
            for (uint32_t tileX = 0U; tileX < control_->tileWidth; ++tileX) {
                const float x0 = static_cast<float>(tileX) * scenario_.width / control_->tileWidth;
                const float x1 = static_cast<float>(tileX + 1U) * scenario_.width / control_->tileWidth;
                if (x1 < minX || x0 > maxX) continue;
                const uint32_t slot = (tileY * control_->tileWidth + tileX) * 4U;
                uint32_t value = 0U;
                for (uint32_t direction = 0U; direction < 4U; ++direction) value = std::max(value, totalPheromone_[slot + direction]);
                const float intensity = std::sqrt(static_cast<float>(value) / static_cast<float>(maximum));
                if (intensity < 0.08F) continue;
                appendRect({34.0F + (x0 - minX) / cameraWidth_ * 570.0F,
                            24.0F + (y0 - minY) / cameraHeight * 455.0F,
                            34.0F + (x1 - minX) / cameraWidth_ * 570.0F,
                            24.0F + (y1 - minY) / cameraHeight * 455.0F},
                           std::array<float, 4>{{0.04F, 0.18F + intensity * 0.20F,
                                                0.24F + intensity * 0.30F, 0.32F}});
            }
        }
    }

    if (cellWidth >= 5.0F && cellHeight >= 5.0F) {
        const uint32_t firstX = static_cast<uint32_t>(std::max(0.0F, std::floor(minX)));
        const uint32_t lastX = std::min(scenario_.width, static_cast<uint32_t>(std::ceil(maxX)));
        const uint32_t firstY = static_cast<uint32_t>(std::max(0.0F, std::floor(minY)));
        const uint32_t lastY = std::min(scenario_.height, static_cast<uint32_t>(std::ceil(maxY)));
        for (uint32_t x = firstX; x <= lastX; ++x) {
            const float screenX = 34.0F + (static_cast<float>(x) - minX) / cameraWidth_ * 570.0F;
            appendLine(screenX, 24.0F, screenX, 479.0F, 0.5F, std::array<float, 4>{{0.2F,0.24F,0.3F,0.45F}});
        }
        for (uint32_t y = firstY; y <= lastY; ++y) {
            const float screenY = 24.0F + (static_cast<float>(y) - minY) / cameraHeight * 455.0F;
            appendLine(34.0F, screenY, 604.0F, screenY, 0.5F, std::array<float, 4>{{0.2F,0.24F,0.3F,0.45F}});
        }
    }

    for (const uint32_t obstacle : scenario_.obstacles) {
        if (vertices_.size() + kUiVertexReserve >= kVertexCapacity) break;
        const uint32_t x = obstacle % scenario_.width;
        const uint32_t y = obstacle / scenario_.width;
        if (static_cast<float>(x + 1U) < minX || static_cast<float>(x) > maxX
            || static_cast<float>(y + 1U) < minY || static_cast<float>(y) > maxY) continue;
        const auto center = gridPoint(obstacle);
        const float halfWidth = std::max(1.5F, cellWidth * 0.5F);
        const float halfHeight = std::max(1.5F, cellHeight * 0.5F);
        appendRect({center[0] - halfWidth, center[1] - halfHeight,
                    center[0] + halfWidth, center[1] + halfHeight},
                   std::array<float, 4>{{0.38F, 0.08F, 0.13F, 1.0F}});
    }

    const bool progressiveFirstRoute = lastProcessedIteration_ == 1U
        && hasAntPlayback_ && antPlaybackProgress_ < 1.0F;
    {
        std::size_t routesRemaining = static_cast<std::size_t>(std::count_if(
            bestRoutes_.begin(), bestRoutes_.end(),
            [&](const auto& route) {
                return !route.empty() && (!progressiveFirstRoute || antPlaybackProgress_ > 0.0F);
            }));
        std::size_t segmentBudget = vertices_.size() + kUiVertexReserve < kVertexCapacity
            ? (kVertexCapacity - kUiVertexReserve - vertices_.size()) / 6U
            : 0U;
        segmentBudget = std::min<std::size_t>(segmentBudget, kMaxVisualRouteSegments);

        for (const auto& route : bestRoutes_) {
            if (route.empty()) continue;
            const std::size_t visibleSteps = progressiveFirstRoute
                ? std::min(route.size(), static_cast<std::size_t>(
                    std::ceil(antPlaybackProgress_ * static_cast<float>(route.size()))))
                : route.size();
            if (visibleSteps == 0U) continue;

            std::vector<uint32_t> bendEndpoints;
            bendEndpoints.reserve(std::min<std::size_t>(visibleSteps, segmentBudget + 1U));
            uint32_t previous = scenario_.source;
            int previousDx = 0;
            int previousDy = 0;
            for (std::size_t step = 0U; step < visibleSteps; ++step) {
                const uint32_t next = route[step];
                const int dx = static_cast<int>(next % scenario_.width) - static_cast<int>(previous % scenario_.width);
                const int dy = static_cast<int>(next / scenario_.width) - static_cast<int>(previous / scenario_.width);
                if (step > 0U && (dx != previousDx || dy != previousDy)) {
                    bendEndpoints.push_back(previous);
                }
                previousDx = dx;
                previousDy = dy;
                previous = next;
            }
            if (bendEndpoints.empty() || bendEndpoints.back() != previous) bendEndpoints.push_back(previous);

            const std::size_t routeBudget = routesRemaining == 0U ? 0U : segmentBudget / routesRemaining;
            const std::size_t segmentsToDraw = std::min(bendEndpoints.size(), routeBudget);
            uint32_t segmentStart = scenario_.source;
            for (std::size_t visualSegment = 1U; visualSegment <= segmentsToDraw; ++visualSegment) {
                const std::size_t endpointIndex =
                    (visualSegment * bendEndpoints.size() + segmentsToDraw - 1U) / segmentsToDraw - 1U;
                const uint32_t segmentEnd = bendEndpoints[endpointIndex];
                const auto a = gridPoint(segmentStart);
                const auto b = gridPoint(segmentEnd);
                appendLine(a[0], a[1], b[0], b[1], 3.5F, kGold);
                segmentStart = segmentEnd;
            }

            segmentBudget -= segmentsToDraw;
            --routesRemaining;
        }
    }

    const std::array<std::array<float, 4>, 5> antColors{{kCyan, kPink, kPurple, kGreen, kGold}};
    for (uint32_t ant = 0U; ant < antDisplayRoutes_.size(); ++ant) {
        const auto& route = antDisplayRoutes_[ant];
        if (route.empty()) continue;
        const float routeProgress = antPlaybackProgress_ * static_cast<float>(route.size());
        const std::size_t segment = std::min<std::size_t>(static_cast<std::size_t>(routeProgress), route.size() - 1U);
        const uint32_t fromNode = segment == 0U
            ? antDisplayRouteStarts_.at(ant) : route[segment - 1U];
        const uint32_t toNode = route[segment];
        const float interpolation = antPlaybackProgress_ >= 1.0F ? 1.0F : routeProgress - std::floor(routeProgress);
        const auto from = gridPoint(fromNode);
        const auto to = gridPoint(toNode);
        const float angle = 2.0F * kPi * static_cast<float>(ant % 10U) / 10.0F;
        const float spread = 1.5F + static_cast<float>(ant / 10U) * 1.2F;
        const float centerX = from[0] + (to[0] - from[0]) * interpolation + std::cos(angle) * spread;
        const float centerY = from[1] + (to[1] - from[1]) * interpolation + std::sin(angle) * spread;
        if (centerX < 30.0F || centerX > 608.0F || centerY < 20.0F || centerY > 483.0F) continue;
        appendCircle(centerX, centerY, 4.2F, 8U, kClear);
        const uint32_t goalIndex = scenario_.goals.empty() ? 0U : ant % static_cast<uint32_t>(scenario_.goals.size());
        appendCircle(centerX, centerY, 2.8F, 8U, antColors[goalIndex % antColors.size()]);
    }

    const auto drawEndpoint = [&](const uint32_t node, const std::array<float, 4>& color, const std::string& label) {
        const auto center = gridPoint(node);
        if (center[0] < 28.0F || center[0] > 610.0F || center[1] < 18.0F || center[1] > 485.0F) return;
        appendCircle(center[0], center[1], std::max(4.5F, std::min(cellWidth, cellHeight) * 0.35F), 12U, color);
        appendText(label, center[0] - 3.0F, center[1] - 4.0F, 1.2F, kClear);
    };
    drawEndpoint(scenario_.source, kGreen, "S");
    for (uint32_t goalIndex = 0U; goalIndex < scenario_.goals.size(); ++goalIndex) {
        drawEndpoint(scenario_.goals[goalIndex], kPink, "G");
    }
    appendText("HORMIGAS  DORADO=RUTA  MAGENTA=OLOR  CIAN=FEROMONA", 18, 492, 1.2F, kMuted);
}

void VulkanApp::drawInteractionGraph() {
    const IterationStats* stats = history_.empty() ? nullptr
        : &history_.at(std::min<std::size_t>(selectedIteration_, history_.size() - 1U));
    std::vector<std::array<float, 2>> positions(antCount_);
    for (uint32_t ant = 0U; ant < antCount_; ++ant) {
        const float angle = -kPi * 0.5F + 2.0F * kPi * static_cast<float>(ant) / static_cast<float>(antCount_);
        positions[ant] = {320.0F + std::cos(angle) * 212.0F, 252.0F + std::sin(angle) * 212.0F};
    }
    float maximum = 0.0F;
    if (stats != nullptr) {
        for (const float value : stats->interaction) maximum = std::max(maximum, value);
    }
    if (stats != nullptr && maximum > 0.0F) {
        for (uint32_t source = 0U; source < antCount_; ++source) {
            for (uint32_t target = 0U; target < antCount_; ++target) {
                if (source == target) continue;
                const float value = stats->interaction[source * antCount_ + target];
                const float normalized = value / maximum;
                if (normalized < 0.12F) continue;
                const auto from = positions[source];
                const auto to = positions[target];
                const std::array<float, 4> color{{kPurple[0], kPurple[1] + normalized * 0.2F,
                                                   kCyan[2], 0.25F + normalized * 0.7F}};
                appendArrow(from[0], from[1], to[0], to[1], 0.8F + normalized * 4.0F, color);
            }
        }
    }
    const uint32_t strongest = stats != nullptr ? stats->influence.mostInfluentialAnt : antCount_;
    for (uint32_t ant = 0U; ant < antCount_; ++ant) {
        const auto center = positions[ant];
        appendCircle(center[0], center[1], ant == strongest ? 10.0F : 7.0F, 14U,
                     ant == strongest ? kGold : kCyan);
        const std::string label = std::to_string(ant);
        appendText(label, center[0] - static_cast<float>(label.size()) * 2.2F, center[1] - 3.0F, 0.9F, kClear);
    }
    appendCircle(320.0F, 252.0F, 72.0F, 32U, kPanel);
    appendText("I(T)", 292, 223, 4.0F, kText);
    appendText(stats != nullptr ? "ITER " + std::to_string(stats->iteration) : "SIN DATOS", 278, 269, 1.8F, kMuted);
    appendText("FLECHA I -> J = INFLUENCIA DE ANT I SOBRE ANT J", 18, 492, 1.25F, kMuted);
}

void VulkanApp::drawMetrics() {
    std::vector<float> best;
    std::vector<float> average;
    std::vector<float> concentration;
    std::vector<float> entropy;
    best.reserve(history_.size());
    average.reserve(history_.size());
    concentration.reserve(history_.size());
    entropy.reserve(history_.size());
    for (std::size_t index = 0U; index < history_.size(); ++index) {
        const IterationStats& stats = history_[index];
        const float totalBest = index < bestTotalHistory_.size() ? bestTotalHistory_[index] : stats.bestCost;
        best.push_back(std::isfinite(totalBest) ? totalBest : 0.0F);
        average.push_back(std::isfinite(stats.averageCost) ? stats.averageCost : 0.0F);
        concentration.push_back(stats.influence.concentration);
        entropy.push_back(stats.influence.entropy);
    }
    appendChart({28, 42, 302, 228}, best, "MEJOR COSTO", kGold);
    appendChart({334, 42, 608, 228}, average, "COSTO PROMEDIO", kCyan);
    appendChart({28, 274, 302, 460}, concentration, "CONCENTRACION C(T)", kPink);
    appendChart({334, 274, 608, 460}, entropy, "ENTROPIA H(T)", kPurple);
    appendText("HISTORIAL COMPLETO GENERADO POR LA GPU", 162, 492, 1.3F, kMuted);
}

void VulkanApp::refreshTitle() const {
    std::ostringstream title;
    const uint32_t visibleGeneration = hasAntPlayback_ && antPlaybackProgress_ < 1.0F
        ? antPlaybackGeneration_
        : (control_ != nullptr ? std::min(control_->iteration + 1U, iterationLimit_) : 1U);
    title << "NetworkACOVulkan | T " << visibleGeneration
          << '/' << iterationLimit_ << " | "
          << (std::isfinite(bestCost_) ? "cost " + std::to_string(bestCost_) : status_);
    glfwSetWindowTitle(window_, title.str().c_str());
}

void VulkanApp::appendRect(const Rect rect, const std::array<float, 4>& color) {
    vertices_.push_back({{ndcX(rect.minX), ndcY(rect.minY)}, color});
    vertices_.push_back({{ndcX(rect.maxX), ndcY(rect.minY)}, color});
    vertices_.push_back({{ndcX(rect.maxX), ndcY(rect.maxY)}, color});
    vertices_.push_back({{ndcX(rect.minX), ndcY(rect.minY)}, color});
    vertices_.push_back({{ndcX(rect.maxX), ndcY(rect.maxY)}, color});
    vertices_.push_back({{ndcX(rect.minX), ndcY(rect.maxY)}, color});
}

void VulkanApp::appendCircle(const float centerX, const float centerY, const float radius,
                             const uint32_t segments, const std::array<float, 4>& color) {
    for (uint32_t index = 0U; index < segments; ++index) {
        const float angle0 = 2.0F * kPi * static_cast<float>(index) / static_cast<float>(segments);
        const float angle1 = 2.0F * kPi * static_cast<float>(index + 1U) / static_cast<float>(segments);
        vertices_.push_back({{ndcX(centerX), ndcY(centerY)}, color});
        vertices_.push_back({{ndcX(centerX + std::cos(angle0) * radius), ndcY(centerY + std::sin(angle0) * radius)}, color});
        vertices_.push_back({{ndcX(centerX + std::cos(angle1) * radius), ndcY(centerY + std::sin(angle1) * radius)}, color});
    }
}

void VulkanApp::appendLine(const float x0, const float y0, const float x1, const float y1,
                           const float width, const std::array<float, 4>& color) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0.0001F) return;
    const float px = -dy / length * width * 0.5F;
    const float py = dx / length * width * 0.5F;
    vertices_.push_back({{ndcX(x0 + px), ndcY(y0 + py)}, color});
    vertices_.push_back({{ndcX(x1 + px), ndcY(y1 + py)}, color});
    vertices_.push_back({{ndcX(x1 - px), ndcY(y1 - py)}, color});
    vertices_.push_back({{ndcX(x0 + px), ndcY(y0 + py)}, color});
    vertices_.push_back({{ndcX(x1 - px), ndcY(y1 - py)}, color});
    vertices_.push_back({{ndcX(x0 - px), ndcY(y0 - py)}, color});
}

void VulkanApp::appendArrow(const float x0, const float y0, const float x1, const float y1,
                            const float width, const std::array<float, 4>& color) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 1.0F) return;
    const float ux = dx / length;
    const float uy = dy / length;
    const float startX = x0 + ux * 11.0F;
    const float startY = y0 + uy * 11.0F;
    const float endX = x1 - ux * 12.0F;
    const float endY = y1 - uy * 12.0F;
    appendLine(startX, startY, endX, endY, width, color);
    const float px = -uy;
    const float py = ux;
    const float arrow = 5.0F + width;
    vertices_.push_back({{ndcX(endX), ndcY(endY)}, color});
    vertices_.push_back({{ndcX(endX - ux * arrow + px * arrow * 0.65F),
                          ndcY(endY - uy * arrow + py * arrow * 0.65F)}, color});
    vertices_.push_back({{ndcX(endX - ux * arrow - px * arrow * 0.65F),
                          ndcY(endY - uy * arrow - py * arrow * 0.65F)}, color});
}

void VulkanApp::appendText(const std::string& text, const float x, const float y, const float pixelSize,
                           const std::array<float, 4>& color) {
    float cursor = x;
    for (const char raw : text) {
        const char value = raw >= 'a' && raw <= 'z' ? static_cast<char>(raw - 'a' + 'A') : raw;
        const auto rows = glyph(value);
        for (uint32_t row = 0U; row < rows.size(); ++row) {
            for (uint32_t column = 0U; column < 5U; ++column) {
                if ((rows[row] & (1U << (4U - column))) == 0U) continue;
                const float x0 = cursor + static_cast<float>(column) * pixelSize;
                const float y0 = y + static_cast<float>(row) * pixelSize;
                appendRect({x0, y0, x0 + pixelSize, y0 + pixelSize}, color);
            }
        }
        cursor += (value == ' ' ? 4.0F : 6.0F) * pixelSize;
    }
}

void VulkanApp::appendChart(const Rect rect, const std::vector<float>& values, const std::string& label,
                            const std::array<float, 4>& color) {
    appendRect(rect, kSidebar);
    appendText(label, rect.minX + 10.0F, rect.minY + 10.0F, 1.6F, kText);
    if (values.size() < 2U) {
        appendText("SIN DATOS", rect.minX + 82.0F, rect.minY + 92.0F, 1.5F, kMuted);
        return;
    }
    const auto [minimumIt, maximumIt] = std::minmax_element(values.begin(), values.end());
    const float minimum = *minimumIt;
    const float maximum = *maximumIt;
    const float range = std::max(maximum - minimum, 1.0e-6F);
    const std::size_t sampleCount = std::min<std::size_t>(values.size(), 512U);
    for (std::size_t sample = 1U; sample < sampleCount; ++sample) {
        const std::size_t previousIndex = (sample - 1U) * (values.size() - 1U) / (sampleCount - 1U);
        const std::size_t currentIndex = sample * (values.size() - 1U) / (sampleCount - 1U);
        const float x0 = rect.minX + 12.0F + static_cast<float>(sample - 1U) * (rect.maxX - rect.minX - 24.0F)
                                           / static_cast<float>(sampleCount - 1U);
        const float x1 = rect.minX + 12.0F + static_cast<float>(sample) * (rect.maxX - rect.minX - 24.0F)
                                           / static_cast<float>(sampleCount - 1U);
        const float y0 = rect.maxY - 14.0F - (values[previousIndex] - minimum) / range
                                           * (rect.maxY - rect.minY - 52.0F);
        const float y1 = rect.maxY - 14.0F - (values[currentIndex] - minimum) / range
                                           * (rect.maxY - rect.minY - 52.0F);
        appendLine(x0, y0, x1, y1, 2.0F, color);
    }
}

bool VulkanApp::contains(const Rect rect, const float x, const float y) const {
    return x >= rect.minX && x <= rect.maxX && y >= rect.minY && y <= rect.maxY;
}

bool VulkanApp::keyPressedOnce(const int key) {
    const bool pressed = glfwGetKey(window_, key) == GLFW_PRESS;
    const bool result = pressed && !keyLatch_[key];
    keyLatch_[key] = pressed;
    return result;
}

VulkanApp::QueueFamilies VulkanApp::findQueueFamilies(const VkPhysicalDevice device) const {
    QueueFamilies result;
    uint32_t count = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());
    for (uint32_t index = 0U; index < count; ++index) {
        const VkQueueFlags required = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        if ((properties[index].queueFlags & required) == required) result.graphicsCompute = index;
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface_, &present);
        if (present == VK_TRUE) result.present = index;
        if (result.complete()) break;
    }
    return result;
}

VulkanApp::SwapSupport VulkanApp::querySwapSupport(const VkPhysicalDevice device) const {
    SwapSupport support;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &support.capabilities);
    uint32_t count = 0U;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &count, nullptr);
    support.formats.resize(count);
    if (count > 0U) vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &count, support.formats.data());
    count = 0U;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &count, nullptr);
    support.presentModes.resize(count);
    if (count > 0U) vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &count, support.presentModes.data());
    return support;
}

bool VulkanApp::deviceSupportsSwapchain(const VkPhysicalDevice device) const {
    uint32_t count = 0U;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
    bool found = false;
    for (const VkExtensionProperties& extension : extensions) {
        if (std::string_view(extension.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME) found = true;
    }
    if (!found) return false;
    const SwapSupport support = querySwapSupport(device);
    return !support.formats.empty() && !support.presentModes.empty();
}

VkSurfaceFormatKHR VulkanApp::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const {
    for (const VkSurfaceFormatKHR format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    if (formats.empty()) throw std::runtime_error("Surface provides no formats.");
    return formats.front();
}

VkPresentModeKHR VulkanApp::choosePresentMode(const std::vector<VkPresentModeKHR>& modes) const {
    for (const VkPresentModeKHR mode : modes) if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApp::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    return {
        std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
}

uint32_t VulkanApp::findMemoryType(const uint32_t typeBits, const VkMemoryPropertyFlags properties) const {
    for (uint32_t index = 0U; index < memoryProperties_.memoryTypeCount; ++index) {
        if ((typeBits & (1U << index)) != 0U
            && (memoryProperties_.memoryTypes[index].propertyFlags & properties) == properties) return index;
    }
    throw std::runtime_error("No compatible host-visible Vulkan memory type.");
}

VkShaderModule VulkanApp::createShaderModule(const std::filesystem::path& path) const {
    const std::vector<char> code = readBinary(path);
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = code.size();
    info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module = VK_NULL_HANDLE;
    throwIfFailed(vkCreateShaderModule(device_, &info, nullptr, &module), "vkCreateShaderModule");
    return module;
}

std::filesystem::path VulkanApp::shaderPath(const std::string& name) const {
#ifdef NETWORK_ACO_SHADER_DIRECTORY
    const std::filesystem::path configured = std::filesystem::path(NETWORK_ACO_SHADER_DIRECTORY) / name;
    if (std::filesystem::exists(configured)) return configured;
#endif
    const std::filesystem::path local = std::filesystem::current_path() / "shaders" / name;
    if (std::filesystem::exists(local)) return local;
    throw std::runtime_error("Shader not found: " + name);
}

VkPipeline VulkanApp::createComputePipeline(const std::string& shaderName) const {
    const VkShaderModule shader = createShaderModule(shaderPath(shaderName));
    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = shader;
    stage.pName = "main";
    VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    info.stage = stage;
    info.layout = computePipelineLayout_;
    VkPipeline pipeline = VK_NULL_HANDLE;
    const VkResult result = vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1U, &info, nullptr, &pipeline);
    vkDestroyShaderModule(device_, shader, nullptr);
    throwIfFailed(result, "vkCreateComputePipelines(" + shaderName + ")");
    return pipeline;
}

void VulkanApp::createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, Buffer& buffer) {
    buffer.size = size;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    throwIfFailed(vkCreateBuffer(device_, &info, nullptr, &buffer.handle), "vkCreateBuffer");
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, buffer.handle, &requirements);
    VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocate.allocationSize = requirements.size;
    allocate.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    throwIfFailed(vkAllocateMemory(device_, &allocate, nullptr, &buffer.memory), "vkAllocateMemory");
    throwIfFailed(vkBindBufferMemory(device_, buffer.handle, buffer.memory, 0U), "vkBindBufferMemory");
    throwIfFailed(vkMapMemory(device_, buffer.memory, 0U, size, 0U, &buffer.mapped), "vkMapMemory");
}

void VulkanApp::destroyBuffer(Buffer& buffer) {
    if (device_ == VK_NULL_HANDLE) return;
    if (buffer.mapped != nullptr) vkUnmapMemory(device_, buffer.memory);
    if (buffer.handle != VK_NULL_HANDLE) vkDestroyBuffer(device_, buffer.handle, nullptr);
    if (buffer.memory != VK_NULL_HANDLE) vkFreeMemory(device_, buffer.memory, nullptr);
    buffer = {};
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int, int) {
    auto* app = static_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) app->framebufferResized_ = true;
}

void VulkanApp::scrollCallback(GLFWwindow* window, double, const double yOffset) {
    auto* app = static_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) app->pendingScroll_ += static_cast<float>(yOffset);
}
