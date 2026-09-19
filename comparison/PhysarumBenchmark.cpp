#include "Scenario.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t kStateCount = 9U;
constexpr uint32_t kInfinity = std::numeric_limits<uint32_t>::max();

void check(const VkResult result, const char* action) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(action) + " (VkResult=" + std::to_string(result) + ')');
    }
}

std::vector<char> readBinary(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Could not open Physarum shader: " + path.string());
    const std::streamsize size = input.tellg();
    input.seekg(0);
    std::vector<char> bytes(static_cast<std::size_t>(size));
    if (size > 0 && !input.read(bytes.data(), size)) {
        throw std::runtime_error("Could not read Physarum shader: " + path.string());
    }
    return bytes;
}

uint32_t parseUint(const std::string& value, const char* field) {
    std::size_t consumed = 0U;
    const unsigned long long parsed = std::stoull(value, &consumed);
    if (consumed != value.size() || parsed > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error(std::string(field) + " must be an unsigned 32-bit integer.");
    }
    return static_cast<uint32_t>(parsed);
}

struct Options {
    std::filesystem::path scenario;
    std::filesystem::path shader;
    uint32_t maxGenerations = 20'000U;
};

Options parseOptions(const int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--scenario" && index + 1 < argc) options.scenario = argv[++index];
        else if (argument == "--shader" && index + 1 < argc) options.shader = argv[++index];
        else if (argument == "--max-generations" && index + 1 < argc) {
            options.maxGenerations = parseUint(argv[++index], "max-generations");
        } else throw std::runtime_error("Unknown or incomplete argument: " + argument);
    }
    if (options.scenario.empty() || options.shader.empty()) {
        throw std::runtime_error("--scenario PATH and --shader PATH are required.");
    }
    if (options.maxGenerations == 0U) throw std::runtime_error("max-generations must be positive.");
    return options;
}

struct Buffer {
    VkBuffer handle = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    VkDeviceSize size = 0U;
};

struct RouteMetrics {
    bool found = false;
    uint64_t costMilli = 0U;
    uint64_t edges = 0U;
    uint64_t networkCells = 0U;
};

class Runner {
public:
    Runner(comparison::Scenario scenario, std::filesystem::path shader)
        : scenario_(std::move(scenario)), shaderPath_(std::move(shader)) {}

    ~Runner() { cleanup(); }

    Runner(const Runner&) = delete;
    Runner& operator=(const Runner&) = delete;

    void initialize() {
        createInstance();
        pickDevice();
        createDevice();
        createBuffers();
        createPipeline();
        createCommands();
        initializeStates();
    }

    struct Result {
        bool stabilized = false;
        uint32_t generations = 0U;
        double seconds = 0.0;
        RouteMetrics route;
        RouteMetrics optimal;
    };

    Result run(const uint32_t maxGenerations) {
        int lastCells = 0;
        int minimumCells = 0;
        uint32_t minimumCheck = 0U;
        bool stabilized = false;
        uint32_t generations = 0U;
        const auto started = std::chrono::steady_clock::now();

        for (uint32_t generation = 1U; generation <= maxGenerations; ++generation) {
            auto* stats = static_cast<uint32_t*>(stats_.mapped);
            stats[0] = stats[1] = stats[2] = 0U;
            dispatch(generation);
            current_ = 1U - current_;
            generations = generation;

            const uint32_t nutrientPending = stats[0];
            const uint32_t nutrientFound = stats[1];
            const int physarumCells = static_cast<int>(stats[2]);
            if (nutrientPending == 0U && nutrientFound > 0U) {
                if (physarumCells < lastCells) minimumCells = physarumCells;
                minimumCheck = minimumCells == lastCells ? minimumCheck + 1U : 0U;
                if (minimumCheck > 10U) {
                    stabilized = true;
                    break;
                }
            }
            lastCells = physarumCells;
        }

        const double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        return {stabilized, generations, seconds, measureRoute(), measureOptimalRoute()};
    }

    [[nodiscard]] const std::string& gpuName() const { return gpuName_; }

private:
    uint32_t memoryType(const uint32_t bits, const VkMemoryPropertyFlags required) const {
        VkPhysicalDeviceMemoryProperties properties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &properties);
        for (uint32_t index = 0U; index < properties.memoryTypeCount; ++index) {
            if ((bits & (1U << index)) != 0U
                && (properties.memoryTypes[index].propertyFlags & required) == required) return index;
        }
        throw std::runtime_error("No host-visible coherent Vulkan memory type supports Physarum buffers.");
    }

    void createInstance() {
        VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        application.pApplicationName = "PhysarumBenchmark";
        application.apiVersion = VK_API_VERSION_1_1;
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        create.pApplicationInfo = &application;
        check(vkCreateInstance(&create, nullptr, &instance_), "Could not create Vulkan instance");
    }

    void pickDevice() {
        uint32_t count = 0U;
        check(vkEnumeratePhysicalDevices(instance_, &count, nullptr), "Could not enumerate Vulkan devices");
        if (count == 0U) throw std::runtime_error("No Vulkan GPU is available.");
        std::vector<VkPhysicalDevice> devices(count);
        check(vkEnumeratePhysicalDevices(instance_, &count, devices.data()), "Could not read Vulkan devices");
        int bestScore = -1;
        for (const VkPhysicalDevice candidate : devices) {
            uint32_t familyCount = 0U;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
            std::vector<VkQueueFamilyProperties> families(familyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
            for (uint32_t family = 0U; family < familyCount; ++family) {
                if ((families[family].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0U) continue;
                VkPhysicalDeviceProperties properties{};
                vkGetPhysicalDeviceProperties(candidate, &properties);
                const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 100 : 10;
                if (score > bestScore) {
                    bestScore = score;
                    physicalDevice_ = candidate;
                    queueFamily_ = family;
                    gpuName_ = properties.deviceName;
                }
                break;
            }
        }
        if (physicalDevice_ == VK_NULL_HANDLE) throw std::runtime_error("No Vulkan compute queue is available.");
    }

    void createDevice() {
        const float priority = 1.0F;
        VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue.queueFamilyIndex = queueFamily_;
        queue.queueCount = 1U;
        queue.pQueuePriorities = &priority;
        VkDeviceCreateInfo create{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        create.queueCreateInfoCount = 1U;
        create.pQueueCreateInfos = &queue;
        check(vkCreateDevice(physicalDevice_, &create, nullptr, &device_), "Could not create Vulkan device");
        vkGetDeviceQueue(device_, queueFamily_, 0U, &queue_);
    }

    void createBuffer(const VkDeviceSize size, Buffer& buffer) {
        VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        create.size = size;
        create.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check(vkCreateBuffer(device_, &create, nullptr, &buffer.handle), "Could not create Physarum buffer");
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device_, buffer.handle, &requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType(
            requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkAllocateMemory(device_, &allocation, nullptr, &buffer.memory), "Could not allocate Physarum buffer");
        check(vkBindBufferMemory(device_, buffer.handle, buffer.memory, 0U), "Could not bind Physarum buffer");
        check(vkMapMemory(device_, buffer.memory, 0U, size, 0U, &buffer.mapped), "Could not map Physarum buffer");
        buffer.size = size;
    }

    void createBuffers() {
        const VkDeviceSize stateSize = static_cast<VkDeviceSize>(scenario_.nodeCount()) * sizeof(uint32_t);
        createBuffer(stateSize, states_[0]);
        createBuffer(stateSize, states_[1]);
        createBuffer(sizeof(uint32_t) * 3U, stats_);
    }

    void createPipeline() {
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
        for (uint32_t binding = 0U; binding < bindings.size(); ++binding) {
            bindings[binding].binding = binding;
            bindings[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings[binding].descriptorCount = 1U;
            bindings[binding].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        VkDescriptorSetLayoutCreateInfo layoutCreate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutCreate.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutCreate.pBindings = bindings.data();
        check(vkCreateDescriptorSetLayout(device_, &layoutCreate, nullptr, &descriptorLayout_),
              "Could not create Physarum descriptor layout");

        VkPushConstantRange push{};
        push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        push.size = sizeof(uint32_t) * 4U;
        VkPipelineLayoutCreateInfo pipelineLayout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayout.setLayoutCount = 1U;
        pipelineLayout.pSetLayouts = &descriptorLayout_;
        pipelineLayout.pushConstantRangeCount = 1U;
        pipelineLayout.pPushConstantRanges = &push;
        check(vkCreatePipelineLayout(device_, &pipelineLayout, nullptr, &pipelineLayout_),
              "Could not create Physarum pipeline layout");

        const std::vector<char> shaderBytes = readBinary(shaderPath_);
        VkShaderModuleCreateInfo moduleCreate{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        moduleCreate.codeSize = shaderBytes.size();
        moduleCreate.pCode = reinterpret_cast<const uint32_t*>(shaderBytes.data());
        VkShaderModule module = VK_NULL_HANDLE;
        check(vkCreateShaderModule(device_, &moduleCreate, nullptr, &module), "Could not create Physarum shader");
        VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage.module = module;
        stage.pName = "main";
        VkComputePipelineCreateInfo pipelineCreate{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        pipelineCreate.stage = stage;
        pipelineCreate.layout = pipelineLayout_;
        const VkResult pipelineResult = vkCreateComputePipelines(
            device_, VK_NULL_HANDLE, 1U, &pipelineCreate, nullptr, &pipeline_);
        vkDestroyShaderModule(device_, module, nullptr);
        check(pipelineResult, "Could not create Physarum compute pipeline");

        VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6U};
        VkDescriptorPoolCreateInfo poolCreate{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolCreate.maxSets = 2U;
        poolCreate.poolSizeCount = 1U;
        poolCreate.pPoolSizes = &poolSize;
        check(vkCreateDescriptorPool(device_, &poolCreate, nullptr, &descriptorPool_),
              "Could not create Physarum descriptor pool");
        const std::array<VkDescriptorSetLayout, 2> layouts{{descriptorLayout_, descriptorLayout_}};
        VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocate.descriptorPool = descriptorPool_;
        allocate.descriptorSetCount = 2U;
        allocate.pSetLayouts = layouts.data();
        check(vkAllocateDescriptorSets(device_, &allocate, descriptors_.data()),
              "Could not allocate Physarum descriptor sets");

        for (uint32_t index = 0U; index < 2U; ++index) {
            const std::array<VkDescriptorBufferInfo, 3> infos{{
                {states_[index].handle, 0U, VK_WHOLE_SIZE},
                {states_[1U - index].handle, 0U, VK_WHOLE_SIZE},
                {stats_.handle, 0U, VK_WHOLE_SIZE},
            }};
            std::array<VkWriteDescriptorSet, 3> writes{};
            for (uint32_t binding = 0U; binding < writes.size(); ++binding) {
                writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[binding].dstSet = descriptors_[index];
                writes[binding].dstBinding = binding;
                writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[binding].descriptorCount = 1U;
                writes[binding].pBufferInfo = &infos[binding];
            }
            vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0U, nullptr);
        }
    }

    void createCommands() {
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool.queueFamilyIndex = queueFamily_;
        check(vkCreateCommandPool(device_, &pool, nullptr, &commandPool_), "Could not create command pool");
        VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocate.commandPool = commandPool_;
        allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate.commandBufferCount = 1U;
        check(vkAllocateCommandBuffers(device_, &allocate, &command_), "Could not allocate command buffer");
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        check(vkCreateFence(device_, &fence, nullptr, &fence_), "Could not create Physarum fence");
    }

    void initializeStates() {
        std::vector<uint32_t> initial(scenario_.nodeCount(), 0U);
        obstacleMask_.assign(scenario_.nodeCount(), 0U);
        for (const uint32_t obstacle : scenario_.obstacles) {
            initial[obstacle] = 2U;
            obstacleMask_[obstacle] = 1U;
        }
        initial[scenario_.source] = 3U;
        for (const uint32_t goal : scenario_.goals) initial[goal] = 1U;
        std::memcpy(states_[0].mapped, initial.data(), initial.size() * sizeof(uint32_t));
        std::memcpy(states_[1].mapped, initial.data(), initial.size() * sizeof(uint32_t));
    }

    void dispatch(const uint32_t generation) {
        check(vkResetCommandBuffer(command_, 0U), "Could not reset Physarum command buffer");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(vkBeginCommandBuffer(command_, &begin), "Could not begin Physarum command buffer");
        VkMemoryBarrier hostBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        hostBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        hostBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(command_, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0U, 1U, &hostBarrier, 0U, nullptr, 0U, nullptr);
        vkCmdBindPipeline(command_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
        vkCmdBindDescriptorSets(command_, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout_,
                                0U, 1U, &descriptors_[current_], 0U, nullptr);
        const std::array<uint32_t, 4> push{{scenario_.width, scenario_.height, generation, 0U}};
        vkCmdPushConstants(command_, pipelineLayout_, VK_SHADER_STAGE_COMPUTE_BIT,
                           0U, sizeof(push), push.data());
        vkCmdDispatch(command_, (scenario_.width + 15U) / 16U, (scenario_.height + 15U) / 16U, 1U);
        VkMemoryBarrier readBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        readBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        readBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(command_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                             0U, 1U, &readBarrier, 0U, nullptr, 0U, nullptr);
        check(vkEndCommandBuffer(command_), "Could not end Physarum command buffer");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1U;
        submit.pCommandBuffers = &command_;
        check(vkQueueSubmit(queue_, 1U, &submit, fence_), "Could not submit Physarum generation");
        check(vkWaitForFences(device_, 1U, &fence_, VK_TRUE, UINT64_MAX), "Could not wait for Physarum generation");
        check(vkResetFences(device_, 1U, &fence_), "Could not reset Physarum fence");
    }

    RouteMetrics shortestRoute(const std::vector<uint8_t>& allowed) const {
        RouteMetrics metrics;
        using Entry = std::pair<uint32_t, uint32_t>;
        const std::array<std::pair<int, int>, 8> offsets{{
            {-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1}
        }};
        metrics.found = true;
        for (const uint32_t goal : scenario_.goals) {
            std::vector<uint32_t> distance(scenario_.nodeCount(), kInfinity);
            std::vector<uint32_t> edges(scenario_.nodeCount(), kInfinity);
            std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
            distance[scenario_.source] = 0U;
            edges[scenario_.source] = 0U;
            queue.emplace(0U, scenario_.source);
            while (!queue.empty()) {
                const auto [cost, current] = queue.top();
                queue.pop();
                if (cost != distance[current]) continue;
                if (current == goal) break;
                const int x = static_cast<int>(current % scenario_.width);
                const int y = static_cast<int>(current / scenario_.width);
                for (const auto& [dx, dy] : offsets) {
                    const int nx = x + dx;
                    const int ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= static_cast<int>(scenario_.width)
                        || ny >= static_cast<int>(scenario_.height)) continue;
                    const uint32_t next = static_cast<uint32_t>(ny) * scenario_.width + static_cast<uint32_t>(nx);
                    if (allowed[next] == 0U) continue;
                    if (dx != 0 && dy != 0) {
                        const uint32_t horizontal = static_cast<uint32_t>(y) * scenario_.width
                                                  + static_cast<uint32_t>(nx);
                        const uint32_t vertical = static_cast<uint32_t>(ny) * scenario_.width
                                                + static_cast<uint32_t>(x);
                        if (obstacleMask_[horizontal] != 0U && obstacleMask_[vertical] != 0U) continue;
                    }
                    const uint32_t candidate = cost + (dx == 0 || dy == 0 ? 1000U : 1414U);
                    if (candidate < distance[next]) {
                        distance[next] = candidate;
                        edges[next] = edges[current] + 1U;
                        queue.emplace(candidate, next);
                    }
                }
            }
            if (distance[goal] == kInfinity) {
                metrics.found = false;
                continue;
            }
            metrics.costMilli += distance[goal];
            metrics.edges += edges[goal];
        }
        return metrics;
    }

    RouteMetrics measureRoute() const {
        const auto* packed = static_cast<const uint32_t*>(states_[current_].mapped);
        std::vector<uint8_t> network(scenario_.nodeCount(), 0U);
        uint64_t networkCells = 0U;
        for (uint32_t node = 0U; node < scenario_.nodeCount(); ++node) {
            const uint32_t state = packed[node] % kStateCount;
            network[node] = state == 3U || state == 4U || state == 5U
                         || state == 6U || state == 7U || state == 8U;
            if (state == 4U || state == 5U || state == 7U || state == 8U) ++networkCells;
        }
        network[scenario_.source] = 1U;
        for (const uint32_t goal : scenario_.goals) network[goal] = 1U;
        RouteMetrics metrics = shortestRoute(network);
        metrics.networkCells = networkCells;
        return metrics;
    }

    RouteMetrics measureOptimalRoute() const {
        std::vector<uint8_t> traversable(scenario_.nodeCount(), 1U);
        for (const uint32_t obstacle : scenario_.obstacles) traversable[obstacle] = 0U;
        return shortestRoute(traversable);
    }

    void destroyBuffer(Buffer& buffer) {
        if (buffer.mapped != nullptr && device_ != VK_NULL_HANDLE) vkUnmapMemory(device_, buffer.memory);
        if (buffer.handle != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) vkDestroyBuffer(device_, buffer.handle, nullptr);
        if (buffer.memory != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) vkFreeMemory(device_, buffer.memory, nullptr);
        buffer = {};
    }

    void cleanup() {
        if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
        if (fence_ != VK_NULL_HANDLE) vkDestroyFence(device_, fence_, nullptr);
        if (commandPool_ != VK_NULL_HANDLE) vkDestroyCommandPool(device_, commandPool_, nullptr);
        if (descriptorPool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        if (pipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(device_, pipeline_, nullptr);
        if (pipelineLayout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        if (descriptorLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device_, descriptorLayout_, nullptr);
        destroyBuffer(states_[0]);
        destroyBuffer(states_[1]);
        destroyBuffer(stats_);
        if (device_ != VK_NULL_HANDLE) vkDestroyDevice(device_, nullptr);
        if (instance_ != VK_NULL_HANDLE) vkDestroyInstance(instance_, nullptr);
        device_ = VK_NULL_HANDLE;
        instance_ = VK_NULL_HANDLE;
    }

    comparison::Scenario scenario_;
    std::filesystem::path shaderPath_;
    std::string gpuName_ = "unknown";
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue queue_ = VK_NULL_HANDLE;
    uint32_t queueFamily_ = 0U;
    std::array<Buffer, 2> states_{};
    Buffer stats_{};
    std::vector<uint8_t> obstacleMask_;
    uint32_t current_ = 0U;
    VkDescriptorSetLayout descriptorLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, 2> descriptors_{};
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer command_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
};

}  // namespace

int main(const int argc, char** argv) {
    try {
        const Options options = parseOptions(argc, argv);
        comparison::Scenario scenario = comparison::loadScenario(options.scenario);
        Runner runner(std::move(scenario), options.shader);
        runner.initialize();
        const Runner::Result result = runner.run(options.maxGenerations);
        const double routeCost = static_cast<double>(result.route.costMilli) / 1000.0;
        const double optimalCost = static_cast<double>(result.optimal.costMilli) / 1000.0;
        const double gap = result.route.found && optimalCost > 0.0
                         ? (routeCost / optimalCost - 1.0) * 100.0
                         : 0.0;
        std::cout << std::fixed << std::setprecision(6)
                  << "BENCHMARK_JSON {\"algorithm\":\"physarum\",\"backend\":\"vulkan-compute\""
                  << ",\"topology\":\"moore-cost-1000-1414-corner-rule\""
                  << ",\"gpu\":\"" << runner.gpuName() << "\""
                  << ",\"success\":" << (result.route.found ? "true" : "false")
                  << ",\"stabilized\":" << (result.stabilized ? "true" : "false")
                  << ",\"generations\":" << result.generations
                  << ",\"route_cost\":" << routeCost
                  << ",\"route_edges\":" << result.route.edges
                  << ",\"optimal_cost\":" << optimalCost
                  << ",\"optimal_route_edges\":" << result.optimal.edges
                  << ",\"gap_percent\":" << gap
                  << ",\"network_cells\":" << result.route.networkCells
                  << ",\"algorithm_seconds\":" << result.seconds << "}\n";
        return result.route.found ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "PhysarumBenchmark error: " << error.what() << '\n';
        return 1;
    }
}
