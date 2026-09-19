#include "Scenario.h"
#include "domain/GridModel.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    std::filesystem::path scenario;
};

Options parseOptions(const int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--scenario" && index + 1 < argc) {
            options.scenario = argv[++index];
        } else {
            throw std::runtime_error("Unknown or incomplete argument: " + argument);
        }
    }
    if (options.scenario.empty()) throw std::runtime_error("--scenario PATH is required.");
    return options;
}

uint32_t stagingTarget(const GridModel& model, const uint32_t source, const uint32_t goal) {
    for (uint32_t node = 0U; node < model.nodeCount(); ++node) {
        if (node != model.source() && node != source && node != goal) return node;
    }
    throw std::runtime_error("Scenario is too small to stage Dijkstra endpoints.");
}

}  // namespace

int main(const int argc, char** argv) {
    try {
        const Options options = parseOptions(argc, argv);
        const comparison::Scenario scenario = comparison::loadScenario(options.scenario);

        uint64_t totalCost = 0U;
        uint64_t totalVisited = 0U;
        uint64_t totalEdges = 0U;
        bool found = true;
        const auto started = std::chrono::steady_clock::now();

        for (const uint32_t goal : scenario.goals) {
            GridModel model({scenario.width, scenario.height});
            model.clearWalls();
            model.setTarget(stagingTarget(model, scenario.source, goal));
            model.setSource(scenario.source);
            model.setTarget(goal);
            for (const uint32_t obstacle : scenario.obstacles) model.setWeight(obstacle, 0U);

            const DijkstraResult result = model.solveCpu();
            found = found && result.found;
            if (!result.found) continue;
            totalCost += result.distance;
            totalVisited += result.visitedNodes;
            totalEdges += result.path.empty() ? 0U : result.path.size() - 1U;
        }

        const double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        std::cout << std::fixed << std::setprecision(6)
                  << "BENCHMARK_JSON {\"algorithm\":\"dijkstra\",\"backend\":\"cpu-priority-queue\""
                  << ",\"success\":" << (found ? "true" : "false")
                  << ",\"width\":" << scenario.width
                  << ",\"height\":" << scenario.height
                  << ",\"goals\":" << scenario.goals.size()
                  << ",\"route_cost\":" << totalCost
                  << ",\"route_edges\":" << totalEdges
                  << ",\"visited_nodes\":" << totalVisited
                  << ",\"algorithm_seconds\":" << elapsed << "}\n";
        return found ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "DijkstraBenchmark error: " << error.what() << '\n';
        return 1;
    }
}

