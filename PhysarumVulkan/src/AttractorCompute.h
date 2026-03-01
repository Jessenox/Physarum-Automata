#pragma once

#include "AttractorGenerator.h"
#include "VulkanHelpers.h"

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

class AttractorCompute {
public:
    struct CreateInfo {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        uint32_t queueFamilyIndex = 0;
        VkQueue queue = VK_NULL_HANDLE;
        std::filesystem::path shaderBinaryPath{};
        std::mutex* queueMutex = nullptr;
    };

    AttractorCompute() = default;
    ~AttractorCompute();

    AttractorCompute(const AttractorCompute&) = delete;
    AttractorCompute& operator=(const AttractorCompute&) = delete;

    bool initialize(const CreateInfo& createInfo);
    void cleanup();

    [[nodiscard]] bool available() const {
        return available_;
    }

    [[nodiscard]] const std::string& status() const {
        return status_;
    }

    void evaluateSuccessors(
        const AttractorSettings& settings,
        const std::vector<AttractorStateBlock>& inputStates,
        std::vector<AttractorStateBlock>& outputStates);

private:
    struct BufferAllocation {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped = nullptr;
        VkDeviceSize size = 0;
    };

    struct PushConstants {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t count = 0;
        uint32_t reserved = 0;
    };

    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    [[nodiscard]] VkShaderModule createShaderModule(const std::vector<char>& code) const;
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, BufferAllocation& allocation);
    void destroyBuffer(BufferAllocation& allocation);
    void createDescriptorResources();
    void createPipeline();
    void createCommandResources();
    void ensureCapacity(std::size_t stateCount);
    void updateDescriptorSet();

private:
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex_ = 0;
    VkQueue queue_ = VK_NULL_HANDLE;
    std::filesystem::path shaderBinaryPath_{};
    std::mutex* queueMutex_ = nullptr;

    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;

    BufferAllocation inputBuffer_{};
    BufferAllocation outputBuffer_{};
    std::size_t capacity_ = 0;
    bool available_ = false;
    std::string status_ = "CPU";
};
