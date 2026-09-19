#pragma once

#include "presentation/ui/DrawGeometry.h"
#include "infrastructure/vulkan/VulkanHelpers.h"

#include <array>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

class AttractorPreviewWindow {
public:
    struct CreateInfo {
        VkInstance instance = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        QueueFamilyIndices queueFamilyIndices{};
        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;
        VkPipelineLayout solidPipelineLayout = VK_NULL_HANDLE;
        std::filesystem::path rectVertexShaderPath;
        std::filesystem::path rectFragmentShaderPath;
        std::mutex* queueSubmitMutex = nullptr;
        int width = 1440;
        int height = 960;
        std::string title = "PhysarumVulkan | Atractores";
        std::array<float, 4> clearColor{{0.03f, 0.04f, 0.06f, 1.0f}};
    };

    struct DrawData {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        uint32_t vertexCount = 0;
        GraphDrawRanges ranges{};
        std::array<float, 4> edgeColor{};
        std::array<float, 4> nodeColor{};
        std::array<float, 4> cycleColor{};
        std::array<float, 4> panelColor{};
        std::array<float, 4> textColor{};
    };

    AttractorPreviewWindow() = default;
    AttractorPreviewWindow(const AttractorPreviewWindow&) = delete;
    AttractorPreviewWindow& operator=(const AttractorPreviewWindow&) = delete;
    AttractorPreviewWindow(AttractorPreviewWindow&&) = delete;
    AttractorPreviewWindow& operator=(AttractorPreviewWindow&&) = delete;
    ~AttractorPreviewWindow();

    void initialize(CreateInfo createInfo);
    void cleanup();
    void open();
    void close();
    [[nodiscard]] bool drawFrame(const DrawData& drawData);
    void setTitle(const std::string& title);

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool shouldClose() const;
    [[nodiscard]] VkExtent2D extent() const;

private:
    struct Vertex {
        float position[2];

        static VkVertexInputBindingDescription bindingDescription();
        static std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions();
    };

    struct RectPushConstants {
        float color[4];
    };

    void createSurface();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createPipeline();
    void createFramebuffers();
    void createCommandResources();
    void createSyncObjects();
    void recreateSwapChain();
    void cleanupSwapChain();
    void recordCommandBuffer(const DrawData& drawData, uint32_t imageIndex);

    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport() const;
    [[nodiscard]] VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    [[nodiscard]] VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char>& code) const;
    [[nodiscard]] VkImageView createImageView(VkImage image, VkFormat format) const;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
    CreateInfo createInfo_{};
    GLFWwindow* window_ = nullptr;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    uint32_t presentFamilyIndex_ = 0;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
    VkFormat swapChainImageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapChainExtent_{};
    std::vector<VkImage> swapChainImages_;
    std::vector<VkImageView> swapChainImageViews_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipeline solidPipeline_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
    VkFence inFlightFence_ = VK_NULL_HANDLE;
    bool framebufferResized_ = false;
    bool initialized_ = false;
};
