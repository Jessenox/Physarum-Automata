#pragma once

#include "domain/NetworkACO.h"
#include "Scenario.h"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <vector>

class VulkanApp {
public:
    explicit VulkanApp(comparison::Scenario scenario, uint32_t iterations = 300U,
                       bool autoVerify = false, bool guidedMode = true,
                       bool odorEnabled = false);
    ~VulkanApp();

    VulkanApp(const VulkanApp&) = delete;
    VulkanApp& operator=(const VulkanApp&) = delete;

    void run();

private:
    static constexpr uint32_t kWindowWidth = 1000U;
    static constexpr uint32_t kWindowHeight = 720U;
    static constexpr uint32_t kBaseAntCount = 30U;
    static constexpr uint32_t kMaxAntCount = 128U;
    static constexpr uint32_t kDefaultIterations = 300U;
    static constexpr uint32_t kIterationCapacity = 10'000U;
    static constexpr uint32_t kMaxGoals = kBaseAntCount;
    static constexpr uint32_t kMaxObstacles = 1'048'576U;
    static constexpr uint32_t kMaxPathCapacity = 262'144U;
    static constexpr uint32_t kPheromoneResolution = 128U;
    static constexpr uint32_t kVertexCapacity = 400'000U;
    static constexpr uint32_t kUiVertexReserve = 100'000U;
    static constexpr uint32_t kMaxVisualRouteSegments = 30'000U;

    struct Buffer {
        VkBuffer handle = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceSize size = 0U;
    };

    struct Vertex {
        std::array<float, 2> position{};
        std::array<float, 4> color{};
    };

    struct Rect {
        float minX = 0.0F;
        float minY = 0.0F;
        float maxX = 0.0F;
        float maxY = 0.0F;
    };

    struct alignas(16) GpuControl {
        uint32_t iteration = 0U;
        uint32_t width = 0U;
        uint32_t height = 0U;
        uint32_t obstacleCount = 0U;
        uint32_t antCount = kBaseAntCount;
        uint32_t maxPath = 0U;
        uint32_t maxIterations = kDefaultIterations;
        uint32_t startNode = 0U;
        uint32_t goalCount = 0U;
        uint32_t tileWidth = 0U;
        uint32_t tileHeight = 0U;
        uint32_t seed = 12345U;
        float alpha = 1.0F;
        float beta = 2.0F;
        float rho = 0.10F;
        float depositQ = 100.0F;
        float initialPheromone = 1.0F;
        uint32_t guidedMode = 1U;
        uint32_t visitedCapacity = 0U;
        uint32_t t1SearchTicks = 0U;
        float odorRadius = 0.0F;
        float odorStrength = 8.0F;
        uint32_t odorEnabled = 0U;
        uint32_t generationSearchTicks = 0U;
    };

    struct alignas(16) GpuGoalBest {
        uint32_t length = 0U;
        uint32_t iteration = 0U;
        float cost = std::numeric_limits<float>::infinity();
        uint32_t ant = 0U;
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

    enum class View : uint32_t { Problem, Interaction, Metrics };
    enum class EditTool : uint32_t { Start, Goal, Obstacle };

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
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorLayout();
    void createPipelineLayouts();
    void createGraphicsPipeline();
    void createComputePipelines();
    void createFramebuffers();
    void createCommandResources();
    void configureAntCount();
    void createBuffers();
    void createDescriptors();
    void createSyncObjects();
    void uploadEditableGrid();
    void recalculateOptimalCosts();

    void waitForFrame();
    void drawFrame();
    void recordCommandBuffer(uint32_t imageIndex, uint32_t iterations, bool resetFirst);
    void computeBarrier(VkAccessFlags sourceAccess, VkAccessFlags destinationAccess);
    void processInput();
    void handleClick(float x, float y);
    void handleGridEdit(float x, float y, bool dragging);
    void changeIterationLimit(int32_t delta);
    void setGuidedMode(bool guided);
    void setOdorEnabled(bool enabled);
    [[nodiscard]] float antPlaybackDurationSeconds() const;
    void updateAntAnimation();
    void captureAntRoutes(uint32_t iteration);
    void resetSimulation();
    void consumeGpuResults();
    void processIteration(uint32_t iteration);
    void rebuildUi();
    void drawProblemGraph();
    void drawInteractionGraph();
    void drawMetrics();
    void refreshTitle() const;

    void appendRect(Rect rect, const std::array<float, 4>& color);
    void appendCircle(float centerX, float centerY, float radius, uint32_t segments,
                      const std::array<float, 4>& color);
    void appendLine(float x0, float y0, float x1, float y1, float width,
                    const std::array<float, 4>& color);
    void appendArrow(float x0, float y0, float x1, float y1, float width,
                     const std::array<float, 4>& color);
    void appendText(const std::string& text, float x, float y, float pixelSize,
                    const std::array<float, 4>& color);
    void appendChart(Rect rect, const std::vector<float>& values, const std::string& label,
                     const std::array<float, 4>& color);
    [[nodiscard]] bool contains(Rect rect, float x, float y) const;
    [[nodiscard]] bool keyPressedOnce(int key);
    [[nodiscard]] uint32_t desiredAntCountForGrid() const;
    [[nodiscard]] uint32_t maxPathForGrid() const;
    [[nodiscard]] uint32_t maxPathCapacityForGrid() const;
    [[nodiscard]] uint32_t visitedCapacityForGrid() const;
    [[nodiscard]] float odorRadiusForGrid() const;
    [[nodiscard]] float odorConcentrationAt(uint32_t node) const;
    [[nodiscard]] std::array<float, 2> gridPoint(uint32_t node) const;
    [[nodiscard]] std::optional<uint32_t> gridNodeAt(float x, float y) const;
    void applyZoom(float amount, float cursorX, float cursorY);

    [[nodiscard]] QueueFamilies findQueueFamilies(VkPhysicalDevice device) const;
    [[nodiscard]] SwapSupport querySwapSupport(VkPhysicalDevice device) const;
    [[nodiscard]] bool deviceSupportsSwapchain(VkPhysicalDevice device) const;
    [[nodiscard]] VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    [[nodiscard]] VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) const;
    [[nodiscard]] VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    [[nodiscard]] uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) const;
    [[nodiscard]] VkShaderModule createShaderModule(const std::filesystem::path& path) const;
    [[nodiscard]] std::filesystem::path shaderPath(const std::string& name) const;
    [[nodiscard]] VkPipeline createComputePipeline(const std::string& shaderName) const;
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, Buffer& buffer);
    void destroyBuffer(Buffer& buffer);

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

    comparison::Scenario scenario_;
    View view_ = View::Problem;
    EditTool editTool_ = EditTool::Start;
    bool autoVerify_ = false;
    bool running_ = false;
    bool pendingReset_ = true;
    bool pendingStep_ = false;
    bool lastSubmissionHadCompute_ = false;
    bool leftMouseWasDown_ = false;
    bool rightMouseWasDown_ = false;
    bool followLatest_ = true;
    bool guidedMode_ = true;
    bool odorEnabled_ = false;
    uint32_t antCount_ = kBaseAntCount;
    uint32_t desiredAntCount_ = kBaseAntCount;
    uint32_t iterationLimit_ = kDefaultIterations;
    float playbackSpeed_ = 1.0F;
    float antPlaybackProgress_ = 1.0F;
    uint32_t antPlaybackMaxSteps_ = 0U;
    uint32_t antPlaybackGeneration_ = 0U;
    double lastAnimationTime_ = 0.0;
    bool hasAntPlayback_ = false;
    bool antPlaybackCompletesGeneration_ = false;
    uint32_t lastProcessedIteration_ = 0U;
    uint32_t lastProcessedSearchTick_ = 0U;
    uint32_t selectedIteration_ = 0U;
    float bestCost_ = std::numeric_limits<float>::infinity();
    std::vector<uint32_t> optimalCosts_;
    uint64_t optimalCostTotal_ = 0U;
    uint32_t bestFoundAt_ = 0U;
    uint32_t bestAnt_ = 0U;
    std::vector<std::vector<uint32_t>> bestRoutes_;
    std::vector<float> bestTotalHistory_;
    std::vector<std::vector<uint32_t>> antDisplayRoutes_;
    std::vector<uint32_t> antDisplayRouteStarts_;
    std::vector<IterationStats> history_;
    std::string status_ = "LISTO";
    std::string gpuName_ = "UNKNOWN";
    std::array<bool, GLFW_KEY_LAST + 1> keyLatch_{};
    uint32_t lastPaintedNode_ = std::numeric_limits<uint32_t>::max();
    float cameraCenterX_ = 0.0F;
    float cameraCenterY_ = 0.0F;
    float cameraWidth_ = 0.0F;
    float pendingScroll_ = 0.0F;
    double lastRightMouseX_ = 0.0;
    double lastRightMouseY_ = 0.0;

    GLFWwindow* window_ = nullptr;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties physicalDeviceProperties_{};
    VkPhysicalDeviceMemoryProperties memoryProperties_{};
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
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout computeDescriptorLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout graphicsPipelineLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout computePipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline initPipeline_ = VK_NULL_HANDLE;
    VkPipeline beginPipeline_ = VK_NULL_HANDLE;
    VkPipeline constructPipeline_ = VK_NULL_HANDLE;
    VkPipeline accumulatePipeline_ = VK_NULL_HANDLE;
    VkPipeline advancePipeline_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkSemaphore imageAvailable_ = VK_NULL_HANDLE;
    VkSemaphore renderFinished_ = VK_NULL_HANDLE;
    VkFence inFlightFence_ = VK_NULL_HANDLE;
    bool framebufferResized_ = false;

    Buffer vertexBuffer_{};
    std::array<Buffer, 19> storageBuffers_{};
    GpuControl* control_ = nullptr;
    uint32_t* totalPheromone_ = nullptr;
    uint32_t* pheromoneByAnt_ = nullptr;
    uint32_t* pathNodes_ = nullptr;
    uint32_t* pathLengths_ = nullptr;
    float* pathCosts_ = nullptr;
    uint32_t* reachedGoal_ = nullptr;
    float* interactionCurrent_ = nullptr;
    float* interactionTotal_ = nullptr;
    float* interactionHistory_ = nullptr;
    uint32_t* bestPaths_ = nullptr;
    GpuGoalBest* goalBest_ = nullptr;
    uint32_t* antStates_ = nullptr;
    uint32_t* pathStarts_ = nullptr;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet computeDescriptorSet_ = VK_NULL_HANDLE;
    std::vector<Vertex> vertices_;
};
