#include "PhysarumSim.h"
#include "VulkanApp.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool containsSnapPath(const std::string_view value) {
    return value.find("/snap/") != std::string_view::npos;
}

std::vector<std::string> splitPathList(const std::string_view value) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= value.size()) {
        const std::size_t separator = value.find(':', start);
        const std::size_t count =
            separator == std::string_view::npos ? value.size() - start : separator - start;
        if (count > 0) {
            parts.emplace_back(value.substr(start, count));
        }
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1;
    }
    return parts;
}

std::string joinPathList(const std::vector<std::string>& parts) {
    std::string result;
    for (const std::string& part : parts) {
        if (part.empty()) {
            continue;
        }
        if (!result.empty()) {
            result.push_back(':');
        }
        result += part;
    }
    return result;
}

void sanitizeGtkEnvironment() {
    const char* gtkPath = std::getenv("GTK_PATH");
    if (gtkPath != nullptr && containsSnapPath(gtkPath)) {
        unsetenv("GTK_PATH");
    }

    const char* gtkModules = std::getenv("GTK_MODULES");
    if (gtkModules != nullptr && *gtkModules != '\0') {
        unsetenv("GTK_MODULES");
    }

    const char* pixbufModuleFile = std::getenv("GDK_PIXBUF_MODULE_FILE");
    if (pixbufModuleFile != nullptr && containsSnapPath(pixbufModuleFile)) {
        unsetenv("GDK_PIXBUF_MODULE_FILE");
    }

    const char* xdgDataDirs = std::getenv("XDG_DATA_DIRS");
    if (xdgDataDirs != nullptr && *xdgDataDirs != '\0') {
        const std::vector<std::string> currentEntries = splitPathList(xdgDataDirs);
        std::vector<std::string> filteredEntries;
        filteredEntries.reserve(currentEntries.size());
        for (const std::string& entry : currentEntries) {
            if (!containsSnapPath(entry)) {
                filteredEntries.push_back(entry);
            }
        }

        const std::string filteredValue = joinPathList(filteredEntries);
        if (filteredValue.empty()) {
            unsetenv("XDG_DATA_DIRS");
        } else if (filteredValue != xdgDataDirs) {
            setenv("XDG_DATA_DIRS", filteredValue.c_str(), 1);
        }
    }
}

GridSize parseGridSize(const std::string& value) {
    const std::size_t separator = value.find('x');
    if (separator == std::string::npos) {
        throw std::runtime_error("Grid format must be WIDTHxHEIGHT.");
    }

    const std::string widthValue = value.substr(0, separator);
    const std::string heightValue = value.substr(separator + 1);
    if (widthValue.empty() || heightValue.empty()) {
        throw std::runtime_error("Grid format must be WIDTHxHEIGHT.");
    }

    const unsigned long parsedWidth = std::stoul(widthValue);
    const unsigned long parsedHeight = std::stoul(heightValue);
    if (parsedWidth == 0 || parsedHeight == 0) {
        throw std::runtime_error("Grid dimensions must be greater than zero.");
    }

    return GridSize{
        static_cast<uint32_t>(parsedWidth),
        static_cast<uint32_t>(parsedHeight)
    };
}

}  // namespace

int main(int argc, char** argv) {
    try {
        sanitizeGtkEnvironment();
        GridSize initialGridSize{200, 200};

        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--grid") {
                if (index + 1 >= argc) {
                    throw std::runtime_error("Missing value after --grid.");
                }
                initialGridSize = parseGridSize(argv[++index]);
                continue;
            }

            if (argument.rfind("--grid=", 0) == 0) {
                initialGridSize = parseGridSize(argument.substr(7));
                continue;
            }

            throw std::runtime_error("Unknown argument: " + argument);
        }

        VulkanApp app(initialGridSize);
        app.run();
    } catch (const std::exception& exception) {
        std::cerr << "PhysarumVulkan fatal error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
