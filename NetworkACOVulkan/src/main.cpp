#include "presentation/VulkanApp.h"
#include "Scenario.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {

uint32_t parseUint32(const std::string& value, const char* field) {
    std::size_t consumed = 0U;
    const unsigned long long parsed = std::stoull(value, &consumed);
    if (consumed != value.size() || parsed > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error(std::string(field) + " must be an unsigned 32-bit integer.");
    }
    return static_cast<uint32_t>(parsed);
}

std::pair<uint32_t, uint32_t> parseGrid(const std::string& value) {
    const std::size_t separator = value.find('x');
    if (separator == std::string::npos) throw std::runtime_error("Grid format must be WIDTHxHEIGHT.");
    const uint32_t width = parseUint32(value.substr(0U, separator), "Grid width");
    const uint32_t height = parseUint32(value.substr(separator + 1U), "Grid height");
    if (width < 8U || height < 8U || width > 10'000U || height > 10'000U) {
        throw std::runtime_error("Network ACO grids must be between 8x8 and 10000x10000.");
    }
    return {width, height};
}

std::pair<uint32_t, uint32_t> parseCoordinate(const std::string& value) {
    const std::size_t separator = value.find(',');
    if (separator == std::string::npos) throw std::runtime_error("Coordinate format must be X,Y.");
    return {
        parseUint32(value.substr(0U, separator), "Coordinate X"),
        parseUint32(value.substr(separator + 1U), "Coordinate Y"),
    };
}

}  // namespace

int main(const int argc, char** argv) {
    try {
        bool verify = false;
        bool guidedMode = true;
        bool odorEnabled = false;
        uint32_t width = 10'000U;
        uint32_t height = 10'000U;
        uint32_t seed = 12345U;
        uint32_t iterations = 300U;
        std::optional<std::filesystem::path> scenarioPath;
        std::optional<std::filesystem::path> savePath;
        std::optional<std::pair<uint32_t, uint32_t>> start;
        std::vector<std::pair<uint32_t, uint32_t>> goals;
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--verify") verify = true;
            else if (argument == "--guided") guidedMode = true;
            else if (argument == "--unguided") guidedMode = false;
            else if (argument == "--smell") odorEnabled = true;
            else if (argument == "--no-smell") odorEnabled = false;
            else if (argument == "--grid" && index + 1 < argc) {
                const auto size = parseGrid(argv[++index]);
                width = size.first;
                height = size.second;
            } else if (argument.rfind("--grid=", 0U) == 0U) {
                const auto size = parseGrid(argument.substr(7U));
                width = size.first;
                height = size.second;
            } else if (argument == "--seed" && index + 1 < argc) {
                seed = parseUint32(argv[++index], "Seed");
            } else if (argument == "--iterations" && index + 1 < argc) {
                iterations = parseUint32(argv[++index], "Iterations");
                if (iterations == 0U || iterations > 10'000U) {
                    throw std::runtime_error("Iterations must be between 1 and 10000.");
                }
            } else if (argument.rfind("--iterations=", 0U) == 0U) {
                iterations = parseUint32(argument.substr(13U), "Iterations");
                if (iterations == 0U || iterations > 10'000U) {
                    throw std::runtime_error("Iterations must be between 1 and 10000.");
                }
            } else if (argument == "--scenario" && index + 1 < argc) {
                scenarioPath = argv[++index];
            } else if (argument == "--save-scenario" && index + 1 < argc) {
                savePath = argv[++index];
            } else if (argument == "--start" && index + 1 < argc) {
                start = parseCoordinate(argv[++index]);
            } else if (argument == "--goal" && index + 1 < argc) {
                goals.push_back(parseCoordinate(argv[++index]));
            } else {
                throw std::runtime_error("Unknown or incomplete argument: " + argument);
            }
        }
        comparison::Scenario scenario = scenarioPath.has_value()
            ? comparison::loadScenario(*scenarioPath)
            : comparison::generateScenario(width, height, seed);
        if (scenario.width > 10'000U || scenario.height > 10'000U) {
            throw std::runtime_error("Network ACO supports grids up to 10000x10000.");
        }
        const auto coordinateNode = [&](const std::pair<uint32_t, uint32_t>& coordinate,
                                        const char* name) {
            if (coordinate.first >= scenario.width || coordinate.second >= scenario.height) {
                throw std::runtime_error(std::string(name) + " coordinate lies outside the grid.");
            }
            return coordinate.second * scenario.width + coordinate.first;
        };
        const auto applyEndpoint = [&](const std::optional<std::pair<uint32_t, uint32_t>>& coordinate,
                                       uint32_t& endpoint, const char* name) {
            if (!coordinate.has_value()) return;
            endpoint = coordinateNode(*coordinate, name);
            const auto obstacle = std::lower_bound(scenario.obstacles.begin(), scenario.obstacles.end(), endpoint);
            if (obstacle != scenario.obstacles.end() && *obstacle == endpoint) scenario.obstacles.erase(obstacle);
        };
        applyEndpoint(start, scenario.source, "Start");
        if (!goals.empty()) {
            scenario.goals.clear();
            for (const auto& coordinate : goals) {
                const uint32_t node = coordinateNode(coordinate, "Goal");
                if (node == scenario.source) throw std::runtime_error("A goal cannot overlap the start.");
                scenario.goals.push_back(node);
                const auto obstacle = std::lower_bound(scenario.obstacles.begin(), scenario.obstacles.end(), node);
                if (obstacle != scenario.obstacles.end() && *obstacle == node) scenario.obstacles.erase(obstacle);
            }
            std::sort(scenario.goals.begin(), scenario.goals.end());
            scenario.goals.erase(std::unique(scenario.goals.begin(), scenario.goals.end()), scenario.goals.end());
        }
        if (scenario.goals.size() > 30U) throw std::runtime_error("Network ACO supports at most 30 goals.");
        scenario.validate();
        if (savePath.has_value()) comparison::saveScenario(scenario, *savePath);
        VulkanApp app(std::move(scenario), iterations, verify, guidedMode, odorEnabled);
        app.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "NetworkACOVulkan error: " << error.what() << '\n';
        return 1;
    }
}
