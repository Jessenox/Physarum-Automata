#include "AttractorCompute.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace {

constexpr std::size_t kStateBlockWords = AttractorStateBlock::kMaxCells;

void throwIfFailed(const VkResult result, const std::string_view action) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(action) + " (VkResult=" + std::to_string(result) + ")");
    }
}

}  // namespace

AttractorCompute::~AttractorCompute() {
    cleanup();
}

bool AttractorCompute::initialize(const CreateInfo& createInfo) {
    cleanup();

    physicalDevice_ = createInfo.physicalDevice;
    device_ = createInfo.device;
    queueFamilyIndex_ = createInfo.queueFamilyIndex;
    queue_ = createInfo.queue;
    shaderBinaryPath_ = createInfo.shaderBinaryPath;
    queueMutex_ = createInfo.queueMutex;

    if (physicalDevice_ == VK_NULL_HANDLE || device_ == VK_NULL_HANDLE || queue_ == VK_NULL_HANDLE) {
        status_ = "ATR CPU";
        return false;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());
    if (queueFamilyIndex_ >= queueFamilies.size() ||
        (queueFamilies[queueFamilyIndex_].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0U) {
        status_ = "ATR CPU";
        return false;
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice_, &deviceFeatures);
    if (deviceFeatures.shaderInt64 != VK_TRUE) {
        status_ = "ATR CPU";
        return false;
    }

    try {
        createDescriptorResources();
        createPipeline();
        createCommandResources();
    } catch (const std::exception&) {
        cleanup();
        status_ = "ATR CPU";
        return false;
    }

    available_ = true;
    status_ = "ATR GPU";
    return true;
}

void AttractorCompute::cleanup() {
    if (device_ != VK_NULL_HANDLE) {
        if (fence_ != VK_NULL_HANDLE) {
            vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX);
        }
    }

    destroyBuffer(inputBuffer_);
    destroyBuffer(outputBuffer_);
    capacity_ = 0;

    if (fence_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, fence_, nullptr);
        fence_ = VK_NULL_HANDLE;
    }

    if (commandPool_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }
    commandBuffer_ = VK_NULL_HANDLE;

    if (pipeline_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (descriptorPool_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }
    descriptorSet_ = VK_NULL_HANDLE;

    available_ = false;
    status_ = "ATR CPU";
    queue_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    queueMutex_ = nullptr;
}

void AttractorCompute::evaluateSuccessors(
    const AttractorSettings& settings,
    const std::vector<AttractorStateBlock>& inputStates,
    std::vector<AttractorStateBlock>& outputStates) {
    if (!available_) {
        throw std::runtime_error("Attractor compute backend is not available.");
    }
    if (inputStates.empty()) {
        outputStates.clear();
        return;
    }

    ensureCapacity(inputStates.size());
    auto* inputWords = static_cast<uint32_t*>(inputBuffer_.mapped);
    for (std::size_t stateIndex = 0; stateIndex < inputStates.size(); ++stateIndex) {
        const std::size_t baseWord = stateIndex * kStateBlockWords;
        for (std::size_t cellIndex = 0; cellIndex < kStateBlockWords; ++cellIndex) {
            inputWords[baseWord + cellIndex] = inputStates[stateIndex].cells[cellIndex];
        }
    }

    throwIfFailed(vkResetFences(device_, 1, &fence_), "Failed to reset attractor compute fence");
    throwIfFailed(vkResetCommandBuffer(commandBuffer_, 0), "Failed to reset attractor compute command buffer");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    throwIfFailed(vkBeginCommandBuffer(commandBuffer_, &beginInfo), "Failed to begin attractor compute command buffer");

    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdBindDescriptorSets(
        commandBuffer_,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout_,
        0,
        1,
        &descriptorSet_,
        0,
        nullptr);

    const PushConstants pushConstants{
        settings.width,
        settings.height,
        static_cast<uint32_t>(inputStates.size()),
        0U
    };
    vkCmdPushConstants(
        commandBuffer_,
        pipelineLayout_,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(PushConstants),
        &pushConstants);

    const uint32_t groupCount = static_cast<uint32_t>((inputStates.size() + 63ULL) / 64ULL);
    vkCmdDispatch(commandBuffer_, std::max(1U, groupCount), 1, 1);

    VkBufferMemoryBarrier outputBarrier{};
    outputBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    outputBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    outputBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    outputBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    outputBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    outputBarrier.buffer = outputBuffer_.buffer;
    outputBarrier.offset = 0;
    outputBarrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(
        commandBuffer_,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0,
        0,
        nullptr,
        1,
        &outputBarrier,
        0,
        nullptr);

    throwIfFailed(vkEndCommandBuffer(commandBuffer_), "Failed to end attractor compute command buffer");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer_;

    if (queueMutex_ != nullptr) {
        std::lock_guard<std::mutex> lock(*queueMutex_);
        throwIfFailed(vkQueueSubmit(queue_, 1, &submitInfo, fence_), "Failed to submit attractor compute work");
    } else {
        throwIfFailed(vkQueueSubmit(queue_, 1, &submitInfo, fence_), "Failed to submit attractor compute work");
    }

    throwIfFailed(vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX), "Failed to wait for attractor compute work");

    outputStates.resize(inputStates.size());
    const auto* outputWords = static_cast<const uint32_t*>(outputBuffer_.mapped);
    for (std::size_t stateIndex = 0; stateIndex < outputStates.size(); ++stateIndex) {
        const std::size_t baseWord = stateIndex * kStateBlockWords;
        for (std::size_t cellIndex = 0; cellIndex < kStateBlockWords; ++cellIndex) {
            outputStates[stateIndex].cells[cellIndex] =
                static_cast<uint8_t>(outputWords[baseWord + cellIndex]);
        }
    }
}

uint32_t AttractorCompute::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);

    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        const bool typeMatches = (typeFilter & (1U << index)) != 0U;
        const bool propertiesMatch =
            (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties;
        if (typeMatches && propertiesMatch) {
            return index;
        }
    }

    throw std::runtime_error("Failed to find attractor compute memory type.");
}

VkShaderModule AttractorCompute::createShaderModule(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    throwIfFailed(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule), "Failed to create attractor compute shader module");
    return shaderModule;
}

void AttractorCompute::createBuffer(
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags properties,
    BufferAllocation& allocation) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throwIfFailed(vkCreateBuffer(device_, &bufferInfo, nullptr, &allocation.buffer), "Failed to create attractor compute buffer");

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device_, allocation.buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    throwIfFailed(vkAllocateMemory(device_, &allocInfo, nullptr, &allocation.memory), "Failed to allocate attractor compute buffer memory");
    throwIfFailed(vkBindBufferMemory(device_, allocation.buffer, allocation.memory, 0), "Failed to bind attractor compute buffer memory");
    throwIfFailed(vkMapMemory(device_, allocation.memory, 0, size, 0, &allocation.mapped), "Failed to map attractor compute buffer");
    allocation.size = size;
}

void AttractorCompute::destroyBuffer(BufferAllocation& allocation) {
    if (allocation.mapped != nullptr && device_ != VK_NULL_HANDLE && allocation.memory != VK_NULL_HANDLE) {
        vkUnmapMemory(device_, allocation.memory);
        allocation.mapped = nullptr;
    }
    if (allocation.buffer != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, allocation.buffer, nullptr);
        allocation.buffer = VK_NULL_HANDLE;
    }
    if (allocation.memory != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, allocation.memory, nullptr);
        allocation.memory = VK_NULL_HANDLE;
    }
    allocation.size = 0;
}

void AttractorCompute::createDescriptorResources() {
    VkDescriptorSetLayoutBinding inputBinding{};
    inputBinding.binding = 0;
    inputBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputBinding.descriptorCount = 1;
    inputBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding outputBinding{};
    outputBinding.binding = 1;
    outputBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputBinding.descriptorCount = 1;
    outputBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    const std::array<VkDescriptorSetLayoutBinding, 2> bindings{{inputBinding, outputBinding}};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    throwIfFailed(
        vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_),
        "Failed to create attractor compute descriptor set layout");

    const std::array<VkDescriptorPoolSize, 1> poolSizes{{
        VkDescriptorPoolSize{
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            2
        }
    }};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;
    throwIfFailed(
        vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_),
        "Failed to create attractor compute descriptor pool");

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &descriptorSetLayout_;
    throwIfFailed(
        vkAllocateDescriptorSets(device_, &allocateInfo, &descriptorSet_),
        "Failed to allocate attractor compute descriptor set");
}

void AttractorCompute::createPipeline() {
    const auto shaderCode = readBinaryFile(shaderBinaryPath_);
    const VkShaderModule shaderModule = createShaderModule(shaderCode);

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout_;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    throwIfFailed(
        vkCreatePipelineLayout(device_, &layoutInfo, nullptr, &pipelineLayout_),
        "Failed to create attractor compute pipeline layout");

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout_;
    throwIfFailed(
        vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_),
        "Failed to create attractor compute pipeline");

    vkDestroyShaderModule(device_, shaderModule, nullptr);
}

void AttractorCompute::createCommandResources() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex_;
    throwIfFailed(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_), "Failed to create attractor compute command pool");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    throwIfFailed(vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer_), "Failed to allocate attractor compute command buffer");

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    throwIfFailed(vkCreateFence(device_, &fenceInfo, nullptr, &fence_), "Failed to create attractor compute fence");
}

void AttractorCompute::ensureCapacity(const std::size_t stateCount) {
    if (stateCount <= capacity_) {
        return;
    }

    if (fence_ != VK_NULL_HANDLE) {
        throwIfFailed(vkWaitForFences(device_, 1, &fence_, VK_TRUE, UINT64_MAX), "Failed to wait for attractor compute resize fence");
    }

    destroyBuffer(inputBuffer_);
    destroyBuffer(outputBuffer_);

    const VkDeviceSize bufferSize = static_cast<VkDeviceSize>(stateCount * kStateBlockWords * sizeof(uint32_t));
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        inputBuffer_);
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        outputBuffer_);

    capacity_ = stateCount;
    updateDescriptorSet();
}

void AttractorCompute::updateDescriptorSet() {
    VkDescriptorBufferInfo inputInfo{};
    inputInfo.buffer = inputBuffer_.buffer;
    inputInfo.offset = 0;
    inputInfo.range = inputBuffer_.size;

    VkDescriptorBufferInfo outputInfo{};
    outputInfo.buffer = outputBuffer_.buffer;
    outputInfo.offset = 0;
    outputInfo.range = outputBuffer_.size;

    const std::array<VkWriteDescriptorSet, 2> writes{{
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet_,
            0,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            &inputInfo,
            nullptr
        },
        VkWriteDescriptorSet{
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptorSet_,
            1,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            nullptr,
            &outputInfo,
            nullptr
        }
    }};

    vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}
