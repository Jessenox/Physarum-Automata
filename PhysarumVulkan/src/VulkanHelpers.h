#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;

    [[nodiscard]] bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    [[nodiscard]] bool hasCompute() const {
        return computeFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

inline std::string deviceTypeToString(const VkPhysicalDeviceType type) {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            return "DISCRETE";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            return "INTEGRATED";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            return "VIRTUAL";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            return "CPU";
        default:
            return "OTHER";
    }
}

inline std::filesystem::path executableDirectory() {
    namespace fs = std::filesystem;

    try {
        return fs::read_symlink("/proc/self/exe").parent_path();
    } catch (...) {
        return fs::current_path();
    }
}

inline std::vector<char> readBinaryFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath.string());
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize < 0) {
        throw std::runtime_error("Failed to determine file size: " + filePath.string());
    }

    std::vector<char> buffer(static_cast<std::size_t>(fileSize));
    file.seekg(0);

    if (!buffer.empty() && !file.read(buffer.data(), fileSize)) {
        throw std::runtime_error("Failed to read file: " + filePath.string());
    }

    return buffer;
}
