#pragma once

#include "application/AppController.h"
#include "presentation/mvvm/AppModel.h"
#include "presentation/mvvm/AppViewModel.h"
#include "presentation/ui/AttractorPreviewWindow.h"
#include "infrastructure/vulkan/AttractorCompute.h"
#include "presentation/ui/DrawGeometry.h"
#include "infrastructure/export/AttractorGraphExporter.h"
#include "presentation/ui/ViewTransform.h"
#include "infrastructure/vulkan/VulkanHelpers.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class VulkanApp {
public:
    explicit VulkanApp(GridSize initialGridSize);

    void run();

private:
    static constexpr uint32_t kWindowWidth = 900;
    static constexpr uint32_t kWindowHeight = 700;
    static constexpr uint32_t kSimulationViewportSize = 500;
    static constexpr std::size_t kMaxFramesInFlight = 1;
    static constexpr double kPartialUploadThreshold = 0.30;
    static constexpr auto kTargetFrameTime = std::chrono::milliseconds(16);
    static constexpr float kMinZoom = 1.0f;
    static constexpr float kMaxZoom = 128.0f;
    static constexpr float kZoomStepMultiplier = 1.35f;
    static constexpr float kPanBlendFactor = 0.35f;
    static constexpr float kPanInertiaDamping = 0.82f;
    static constexpr float kPanVelocityEpsilon = 0.00002f;

#ifdef NDEBUG
    static constexpr bool kEnableValidationLayers = false;
#else
    static constexpr bool kEnableValidationLayers = true;
#endif

    struct BufferAllocation {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceSize size = 0;
    };

    struct ImageAllocation {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    enum class UploadMode {
        None,
        Full,
        Partial,
    };

    struct UploadRequest {
        UploadMode mode = UploadMode::None;
        DirtyRegion region{};
    };

    struct QuadPushConstants {
        float uvMin[2];
        float uvMax[2];
        uint32_t gridWidth = 0;
        uint32_t gridHeight = 0;
    };

    struct ScreenRect {
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;
    };

    struct UiElement {
        ScreenRect rect{};
        SolidDrawRange draw{};
    };

    struct ColorAdjustButtons {
        UiElement decrement{};
        UiElement increment{};
    };

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    void processInput();
    void updateSimulation();
    void updateAttractorState();
    [[nodiscard]] std::chrono::milliseconds targetSimulationInterval() const;
    [[nodiscard]] ViewportRect simulationViewportRect() const;
    [[nodiscard]] bool isInsideSimulationArea(double mouseX, double mouseY) const;
    void resetView();
    void clampView();
    void applyPendingZoom();
    void zoomAtCursor(double mouseX, double mouseY, float zoomMultiplier);
    [[nodiscard]] bool updatePan();
    void updateViewMotion();
    void updateCursorFeedback(double mouseX, double mouseY);
    [[nodiscard]] QuadPushConstants currentQuadPushConstants() const;
    [[nodiscard]] std::pair<float, float> screenToLogical(double mouseX, double mouseY) const;
    [[nodiscard]] std::pair<uint32_t, uint32_t> screenToCell(double mouseX, double mouseY) const;
    [[nodiscard]] bool isInsideSidebarScrollableArea(float logicalX, float logicalY) const;
    [[nodiscard]] float maxSidebarScrollOffset() const;
    void nudgeSidebarScroll(float delta);
    void handleSidebarClick(float logicalX, float logicalY);
    void nudgeSelectedStateColor(std::size_t channel, int delta);
    void nudgeAttractorDimension(bool adjustWidth, int delta);
    void requestGridResize(GridSize newSize);
    void refreshWindowTitle() const;
    void logGridConfiguration() const;
    void validateGridSize(GridSize size) const;
    void resetAttractorPreviewBounds();
    void openAttractorPreviewWindow();
    void closeAttractorPreviewWindow();
    void updateAttractorPreviewWindow();
    void renderAttractorPreview(const AttractorGraph& graph);
    void renderAttractorStatusPreview(const std::string& statusText);
    void exportAttractorSvg();
    void exportAttractorPng();
    [[nodiscard]] BatchSuccessorEvaluator currentSuccessorEvaluator();

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createCommandPool();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createPipelineLayouts();
    void createGraphicsPipelines();
    void createFramebuffers();
    void createVertexBuffers();
    void createOverlayTextBuffer();
    void createAttractorGraphBuffer();
    void createSimulationBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void updateDescriptorSets();
    void createStagingBuffers();
    void createComputeDescriptorSetLayout();
    void createComputePipeline();
    void createCommandBuffers();
    void createSyncObjects();
    void recreateSwapChain();
    void cleanupSwapChain();
    void destroySimulationBuffers();
    void destroyBuffer(BufferAllocation& allocation);
    void rebuildSolidUiBuffer();
    void updateOverlayTextBuffer();
    void rebuildAttractorGraphBuffer(const AttractorGraph& graph);

    [[nodiscard]] UploadRequest prepareSimulationUpload(uint32_t frameIndex);
    void packFullSimulationToStaging(void* destination) const;
    void packDirtyRegionToStaging(const DirtyRegion& region, void* destination) const;
    void consumeGpuSimulationResults();
    void updateGpuRoutingState(uint32_t nutrientPending, uint32_t nutrientFound, uint32_t physarumCells);
    void recordCommandBuffer(
        VkCommandBuffer commandBuffer,
        uint32_t imageIndex,
        uint32_t frameIndex,
        const UploadRequest& uploadRequest);
    void drawFrame();
    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice deviceHandle) const;
    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice deviceHandle, VkSurfaceKHR surface) const;
    [[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice deviceHandle) const;
    [[nodiscard]] bool checkDeviceExtensionSupport(VkPhysicalDevice deviceHandle) const;
    [[nodiscard]] bool checkValidationLayerSupport() const;
    [[nodiscard]] std::vector<const char*> getRequiredExtensions() const;
    [[nodiscard]] VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    [[nodiscard]] VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) const;
    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char>& code) const;
    [[nodiscard]] VkImageView createImageView(VkImage image, VkFormat format) const;
    [[nodiscard]] std::filesystem::path shaderPath(const std::string& name) const;

    void createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        BufferAllocation& allocation);
    void createImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        ImageAllocation& allocation);
    void copyBuffer(const BufferAllocation& source, const BufferAllocation& destination, VkDeviceSize size);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void* userData);

private:
    GLFWwindow* window_ = nullptr;
    GLFWcursor* panCursor_ = nullptr;
    AppModel model_;
    AppViewModel viewModel_;
    AppController controller_;
    PhysarumSim& simulation_;
    GridSize& requestedGridSize_;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties_{};
    QueueFamilyIndices queueFamilyIndices_{};

    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue computeQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages_;
    VkFormat swapChainImageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent_{};
    std::vector<VkImageView> swapChainImageViews_;
    std::vector<VkFramebuffer> swapChainFramebuffers_;

    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout texturedPipelineLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout solidPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline texturedPipeline_ = VK_NULL_HANDLE;
    VkPipeline solidPipeline_ = VK_NULL_HANDLE;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, kMaxFramesInFlight> commandBuffers_{};

    BufferAllocation texturedVertexBuffer_{};
    BufferAllocation solidVertexBuffer_{};
    BufferAllocation overlayTextVertexBuffer_{};
    BufferAllocation attractorGraphVertexBuffer_{};
    std::array<BufferAllocation, kMaxFramesInFlight> stagingBuffers_{};
    SolidDrawRange panelDraw_{};
    SolidDrawRange sidebarDraw_{};
    SolidDrawRange indicatorDraw_{};
    SolidDrawRange scrollbarTrackDraw_{};
    SolidDrawRange scrollbarThumbDraw_{};
    uint32_t overlayTextVertexCount_ = 0;
    uint32_t attractorGraphVertexCount_ = 0;
    GraphDrawRanges attractorGraphDraws_{};
    UiElement loadMapButton_{};
    UiElement attractorsButton_{};
    UiElement selectedColorPreview_{};
    std::array<UiElement, 9> stateButtons_{};
    std::array<UiElement, 9> stateSwatches_{};
    std::array<ColorAdjustButtons, 3> colorAdjustButtons_{};
    ColorAdjustButtons attractorWidthButtons_{};
    ColorAdjustButtons attractorHeightButtons_{};
    UiElement attractorRefineButton_{};
    UiElement attractorExportButton_{};
    UiElement attractorExportPngButton_{};

    std::array<BufferAllocation, 2> simulationStateBuffers_{};
    BufferAllocation simulationPaletteBuffer_{};
    BufferAllocation simulationStatsBuffer_{};
    uint32_t currentSimulationBufferIndex_ = 0;
    bool simulationPaletteDirty_ = true;
    bool simulationGpuEnabled_ = false;
    bool simulationGraphicsQueueComputeCapable_ = false;
    bool pendingGpuSimulationStep_ = false;
    bool gpuStepSubmitted_ = false;
    int gpuPhysarumLastCells_ = 0;
    int gpuMinimumPhysarumCells_ = 0;
    int gpuMinimumCheck_ = 0;

    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, 2> descriptorSets_{};
    VkDescriptorSetLayout computeDescriptorSetLayout_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, 2> computeDescriptorSets_{};
    VkPipelineLayout computePipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline computePipeline_ = VK_NULL_HANDLE;

    std::array<VkSemaphore, kMaxFramesInFlight> imageAvailableSemaphores_{};
    std::array<VkSemaphore, kMaxFramesInFlight> renderFinishedSemaphores_{};
    std::array<VkFence, kMaxFramesInFlight> inFlightFences_{};
    std::vector<VkFence> imagesInFlight_;

    std::vector<const char*> validationLayers_ = {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char*> requiredDeviceExtensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    bool validationLayersEnabled_ = false;
    bool framebufferResized_ = false;

    std::string selectedGpuName_ = "Unknown";
    std::string selectedGpuType_ = "OTHER";
    bool shaderInt64Supported_ = false;
    std::mutex queueSubmitMutex_{};

    std::array<bool, GLFW_KEY_LAST + 1> keyPressed_{};

    uint8_t& selectedState_;
    ViewTransform viewTransform_{ViewTransform::Config{
        kMinZoom,
        kMaxZoom,
        kPanBlendFactor,
        kPanInertiaDamping,
        kPanVelocityEpsilon
    }};
    double pendingZoomDelta_ = 0.0;
    float mouseLogicalX_ = -1.0f;
    float mouseLogicalY_ = -1.0f;
    float& sidebarScrollOffset_;
    bool leftMousePressed_ = false;
    bool uiMouseCapture_ = false;
    MenuStatus& menuStatus_;
    bool& showingAttractorGraph_;
    AttractorSettings& attractorSettings_;
    AttractorGraphExporter attractorExporter_{};
    AttractorCompute attractorCompute_{};
    AttractorProgress& attractorProgress_;
    std::optional<AttractorGraph>& latestAttractorGraph_;
    AttractorPreviewWindow attractorPreviewWindow_{};
    bool attractorPreviewBoundsInitialized_ = false;
    bool attractorPreviewRenderCapped_ = false;
    float attractorPreviewWorldMinX_ = 0.0f;
    float attractorPreviewWorldMinY_ = 0.0f;
    float attractorPreviewWorldMaxX_ = 0.0f;
    float attractorPreviewWorldMaxY_ = 0.0f;
    std::string& attractorComputeStatus_;

    std::size_t currentFrame_ = 0;
};
