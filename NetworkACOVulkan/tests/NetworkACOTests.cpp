#include "domain/Graph.h"
#include "domain/NetworkACO.h"
#include "Scenario.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <numeric>
#include <filesystem>

namespace {

bool close(const float a, const float b, const float tolerance = 1.0e-5F) {
    return std::abs(a - b) <= tolerance;
}

WeightedGraph smallGraph() {
    WeightedGraph graph;
    graph.addNode(0.0F, 0.5F);
    graph.addNode(0.4F, 0.2F);
    graph.addNode(0.4F, 0.8F);
    graph.addNode(1.0F, 0.5F);
    graph.addUndirectedEdge(0U, 1U, 2.0F);
    graph.addUndirectedEdge(1U, 3U, 2.0F);
    graph.addUndirectedEdge(0U, 2U, 5.0F);
    graph.addUndirectedEdge(2U, 3U, 1.0F);
    graph.addUndirectedEdge(1U, 2U, 2.5F);
    return graph;
}

void testProbabilities() {
    const WeightedGraph graph = smallGraph();
    const PheromoneState pheromone(static_cast<uint32_t>(graph.edges().size()), 3U, 1.0F);
    const auto probabilities = ACOEngine::selectionProbabilities(
        graph, pheromone, graph.neighbors(0U), 1.0F, 2.0F);
    assert(probabilities.size() == 2U);
    assert(close(std::accumulate(probabilities.begin(), probabilities.end(), 0.0F), 1.0F));
    assert(probabilities[0] > probabilities[1]);
}

void testPheromoneOriginAndEvaporation() {
    PheromoneState pheromone(2U, 3U, 1.0F);
    const uint32_t edge = 0U;
    pheromone.deposit(0U, std::span<const uint32_t>(&edge, 1U), 10.0F);
    assert(close(pheromone.contribution(0U, 0U), 10.0F));
    assert(close(pheromone.contribution(0U, 1U), 0.0F));
    assert(close(pheromone.total(0U), 11.0F));
    pheromone.evaporate(0.1F);
    assert(close(pheromone.contribution(0U, 0U), 9.0F));
    assert(close(pheromone.total(0U), 9.9F));
}

void testInfluenceAndNoSelfInfluence() {
    PheromoneState pheromone(1U, 3U, 1.0F);
    const uint32_t edge = 0U;
    pheromone.deposit(0U, std::span<const uint32_t>(&edge, 1U), 0.7F);
    InteractionNetwork interaction(3U);
    interaction.beginIteration();
    interaction.recordTraversal(1U, 0U, pheromone);
    interaction.endIteration();
    assert(close(interaction.current()[0U * 3U + 1U], 0.7F));
    assert(close(interaction.current()[1U * 3U + 1U], 0.0F));
    assert(close(interaction.total()[0U * 3U + 1U], 0.7F));
}

void testPathCost() {
    const WeightedGraph graph = smallGraph();
    const uint32_t path[] = {0U, 1U};
    assert(close(graph.pathCost(0U, path), 4.0F));
    const auto nodes = graph.nodePath(0U, path);
    assert((nodes == std::vector<uint32_t>{0U, 1U, 3U}));
}

void testSeedReproducibility() {
    ACOConfig config;
    config.ants = 12U;
    config.iterations = 20U;
    config.seed = 98765U;
    const ACOResult first = ACOEngine(smallGraph(), 0U, 3U, config).run();
    const ACOResult second = ACOEngine(smallGraph(), 0U, 3U, config).run();
    assert(close(first.bestCost, second.bestCost));
    assert(first.bestEdges == second.bestEdges);
    assert(first.history.size() == second.history.size());
    for (std::size_t index = 0U; index < first.history.size(); ++index) {
        assert(close(first.history[index].averageCost, second.history[index].averageCost));
        assert(first.history[index].successfulAnts == second.history[index].successfulAnts);
        assert(first.history[index].interaction == second.history[index].interaction);
    }
}

void testSharedGridScenario() {
    comparison::Scenario generated = comparison::generateScenario(32U, 24U, 12345U);
    generated.goals.push_back(22U * generated.width + 30U);
    std::sort(generated.goals.begin(), generated.goals.end());
    generated.obstacles = {2U * generated.width + 2U, 3U * generated.width + 3U};
    generated.validate();
    const WeightedGraph grid = WeightedGraph::makeGridNetwork(generated);
    assert(grid.nodes().size() == generated.nodeCount());
    assert(!grid.edges().empty());
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "network-aco-test.pacgrid";
    comparison::saveScenario(generated, path);
    const comparison::Scenario loaded = comparison::loadScenario(path);
    std::filesystem::remove(path);
    assert(loaded.width == generated.width);
    assert(loaded.height == generated.height);
    assert(loaded.source == generated.source);
    assert(loaded.seed == generated.seed);
    assert(loaded.goals == generated.goals);
    assert(loaded.obstacles == generated.obstacles);

    const comparison::Scenario large = comparison::generateScenario(10'000U, 10'000U, 7U);
    assert(large.nodeCount() == 100'000'000U);
    assert(large.obstacles.empty());
    assert(large.goals.size() == 1U);
}

}  // namespace

int main() {
    testProbabilities();
    testPheromoneOriginAndEvaporation();
    testInfluenceAndNoSelfInfluence();
    testPathCost();
    testSeedReproducibility();
    testSharedGridScenario();
    std::cout << "Network ACO domain tests passed.\n";
    return 0;
}
