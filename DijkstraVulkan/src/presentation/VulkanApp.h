#pragma once

#include "domain/GridModel.h"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

inline constexpr uint64_t kDijkstraInteractiveNodeLimit = 1'048'576ULL;

class VulkanApp {
public:
    explicit VulkanApp(GridSize size, bool autoVerify = false);
    ~VulkanApp();

    VulkanApp(const VulkanApp&) = delete;
    VulkanApp& operator=(const VulkanApp&) = delete;

    void run();

private:
    static constexpr uint32_t kWindowWidth = 900;
    static constexpr uint32_t kWindowHeight = 700;
    static constexpr uint32_t kCanvasSize = 500;
    static constexpr uint32_t kInvalidNode = 0xffffffffU;
    static constexpr uint32_t kUiVertexCapacity = 180'000U;

    struct Buffer {
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceSize size = 0;
    };

    struct TexturedVertex {
        float position[2];
        float uv[2];
    };

    struct UiVertex {
        float position[2];
    };

    struct DrawItem {
        uint32_t firstVertex = 0;
        uint32_t vertexCount = 0;
        std::array<float, 4> color{};
    };

    struct Rect {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
    };

    struct GpuControl {
        uint32_t minDistance = kInvalidNode;
        uint32_t selected = kInvalidNode;
        uint32_t finished = 0;
        uint32_t found = 0;
        uint32_t iterations = 0;
        uint32_t source = 0;
        uint32_t target = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    struct QueueFamilies {
        std::optional<uint32_t> graphicsCompute;
        std::optional<uint32_t> present;
        [[nodiscard]] bool complete() const { return graphicsCompute.has_value() && present.has_value(); }
    };

    struct SwapSupport {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    enum class PaintTool : uint32_t {
        Source,
        Target,
        Wall,
        Weight1,
        Weight2,
        Weight5,
    };

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    void cleanupSwapchain();
    void recreateSwapchain();

    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createDevice();
    void initializeGridModel();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorLayouts();
    void createPipelineLayouts();
    void createGraphicsPipelines();
    void createComputePipelines();
    void createFramebuffers();
    void createCommandResources();
    void createBuffers();
    void createDescriptors();
    void createSyncObjects();

    void waitForFrame();
    void drawFrame();
    void recordCommandBuffer(uint32_t imageIndex, uint32_t computeSteps, bool resetFirst);
    void recordComputeBarrier(VkAccessFlags sourceAccess, VkAccessFlags destinationAccess);
    void processInput();
    void handleSidebarClick(float x, float y);
    void paintCell(uint32_t index);
    void resetSearch(const std::string& status);
    void uploadModel();
    void uploadCell(uint32_t index);
    void consumeGpuResult();
    void rebuildPath();
    void rebuildUi();
    void refreshTitle() const;

    void appendRect(Rect rect, const std::array<float, 4>& color);
    void appendText(const std::string& text, float x, float y, float pixelSize, const std::array<float, 4>& color);
    [[nodiscard]] bool contains(Rect rect, float x, float y) const;
    [[nodiscard]] std::optional<uint32_t> cellAtCursor(double mouseX, double mouseY) const;
    [[nodiscard]] bool keyPressedOnce(int key);
    [[nodiscard]] uint32_t safeStepsPerFrame() const;

    [[nodiscard]] QueueFamilies findQueueFamilies(VkPhysicalDevice device) const;
    [[nodiscard]] SwapSupport querySwapSupport(VkPhysicalDevice device) const;
    [[nodiscard]] bool deviceSupportsSwapchain(VkPhysicalDevice device) const;
    [[nodiscard]] VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    [[nodiscard]] VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) const;
    [[nodiscard]] VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    [[nodiscard]] uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
    [[nodiscard]] VkShaderModule createShaderModule(const std::filesystem::path& path) const;
    [[nodiscard]] std::filesystem::path shaderPath(const std::string& name) const;
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Buffer& buffer);
    void destroyBuffer(Buffer& buffer);
    [[nodiscard]] VkPipeline createComputePipeline(const std::string& shaderName) const;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GridSize requestedGridSize_{};
    std::unique_ptr<GridModel> model_;
    PaintTool tool_ = PaintTool::Wall;
    bool running_ = false;
    bool pendingReset_ = true;
    uint32_t pendingSteps_ = 0;
    uint32_t stepsPerFrame_ = 8;
    bool resultConsumed_ = false;
    bool leftMouseWasDown_ = false;
    uint32_t lastPaintedCell_ = kInvalidNode;
    std::array<bool, GLFW_KEY_LAST + 1> keyLatch_{};
    std::string status_ = "LISTO";
    std::string gpuName_ = "UNKNOWN";
    bool autoVerify_ = false;

    GLFWwindow* window_ = nullptr;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties_{};
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties_{};
    VkMemoryPropertyFlags storageMemoryProperties_ =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint32_t storageMemoryHeapIndex_ = 0;
    uint64_t hardwareNodeCapacity_ = 0;
    uint64_t maximumNodeCount_ = 0;
    QueueFamilies queueFamilies_{};
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapchainFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent_{};
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    std::vector<VkFramebuffer> framebuffers_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;

    VkDescriptorSetLayout gridDescriptorLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout computeDescriptorLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout gridPipelineLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout uiPipelineLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout computePipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline gridPipeline_ = VK_NULL_HANDLE;
    VkPipeline uiPipeline_ = VK_NULL_HANDLE;
    VkPipeline resetPipeline_ = VK_NULL_HANDLE;
    VkPipeline clearPipeline_ = VK_NULL_HANDLE;
    VkPipeline findMinPipeline_ = VK_NULL_HANDLE;
    VkPipeline chooseMinPipeline_ = VK_NULL_HANDLE;
    VkPipeline relaxPipeline_ = VK_NULL_HANDLE;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkSemaphore imageAvailable_ = VK_NULL_HANDLE;
    VkSemaphore renderFinished_ = VK_NULL_HANDLE;
    VkFence inFlightFence_ = VK_NULL_HANDLE;
    bool framebufferResized_ = false;

    Buffer gridVertexBuffer_{};
    Buffer uiVertexBuffer_{};
    Buffer cellsBuffer_{};
    Buffer distancesBuffer_{};
    Buffer previousBuffer_{};
    Buffer visitedBuffer_{};
    Buffer controlBuffer_{};
    GpuControl* control_ = nullptr;
    uint32_t* distances_ = nullptr;
    uint32_t* previous_ = nullptr;
    uint32_t* packedCells_ = nullptr;
    uint32_t* packedVisited_ = nullptr;

    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet gridDescriptorSet_ = VK_NULL_HANDLE;
    VkDescriptorSet computeDescriptorSet_ = VK_NULL_HANDLE;
    std::vector<UiVertex> uiVertices_;
    std::vector<DrawItem> uiDrawItems_;
};
