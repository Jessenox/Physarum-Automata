#include "domain/GridModel.h"
#include "presentation/VulkanApp.h"

#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

GridSize parseGridSize(const std::string& value) {
    const std::size_t separator = value.find('x');
    if (separator == std::string::npos) {
        throw std::runtime_error("Grid format must be WIDTHxHEIGHT.");
    }
    const unsigned long long width = std::stoull(value.substr(0, separator));
    const unsigned long long height = std::stoull(value.substr(separator + 1));
    if (width < 2 || height < 2 ||
        width > std::numeric_limits<uint32_t>::max() ||
        height > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("Grid dimensions must fit unsigned 32-bit values and be at least 2x2.");
    }
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

}  // namespace

int main(int argc, char** argv) {
    try {
        GridSize grid{64, 64};
        bool verify = false;
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--grid" && index + 1 < argc) {
                grid = parseGridSize(argv[++index]);
            } else if (argument.rfind("--grid=", 0) == 0) {
                grid = parseGridSize(argument.substr(7));
            } else if (argument == "--verify") {
                verify = true;
            } else {
                throw std::runtime_error("Unknown argument: " + argument);
            }
        }
        const uint64_t requestedNodes = static_cast<uint64_t>(grid.width) * grid.height;
        if (requestedNodes > kDijkstraInteractiveNodeLimit) {
            throw std::runtime_error(
                "The dense interactive Dijkstra backend is safety-limited to 1048576 nodes "
                "(1024x1024). The requested grid would monopolize RAM/VRAM and the desktop GPU.");
        }
        VulkanApp app(grid, verify);
        app.run();
    } catch (const std::exception& exception) {
        std::cerr << "DijkstraVulkan fatal error: " << exception.what() << '\n';
        return 1;
    }
    return 0;
}
