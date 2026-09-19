#include "domain/Graph.h"

#include "Scenario.h"

#include <cmath>
#include <stdexcept>

uint32_t WeightedGraph::addNode(const float x, const float y) {
    const uint32_t index = static_cast<uint32_t>(nodes_.size());
    nodes_.push_back({x, y});
    adjacency_.emplace_back();
    return index;
}

uint32_t WeightedGraph::addUndirectedEdge(const uint32_t from, const uint32_t to, const float cost) {
    if (from >= nodes_.size() || to >= nodes_.size() || from == to) {
        throw std::invalid_argument("Invalid graph edge endpoints.");
    }
    if (!std::isfinite(cost) || cost <= 0.0F) throw std::invalid_argument("Edge cost must be positive.");
    const uint32_t index = static_cast<uint32_t>(edges_.size());
    edges_.push_back({from, to, cost});
    adjacency_[from].push_back({to, index});
    adjacency_[to].push_back({from, index});
    return index;
}

const std::vector<GraphNode>& WeightedGraph::nodes() const { return nodes_; }
const std::vector<GraphEdge>& WeightedGraph::edges() const { return edges_; }

const std::vector<GraphAdjacency>& WeightedGraph::neighbors(const uint32_t node) const {
    return adjacency_.at(node);
}

float WeightedGraph::pathCost(const uint32_t start, const std::span<const uint32_t> edgePath) const {
    uint32_t current = start;
    float total = 0.0F;
    for (const uint32_t edgeIndex : edgePath) {
        const GraphEdge& edge = edges_.at(edgeIndex);
        if (edge.from == current) current = edge.to;
        else if (edge.to == current) current = edge.from;
        else throw std::invalid_argument("Edge path is not contiguous.");
        total += edge.cost;
    }
    return total;
}

std::vector<uint32_t> WeightedGraph::nodePath(const uint32_t start,
                                              const std::span<const uint32_t> edgePath) const {
    std::vector<uint32_t> result{start};
    uint32_t current = start;
    for (const uint32_t edgeIndex : edgePath) {
        const GraphEdge& edge = edges_.at(edgeIndex);
        if (edge.from == current) current = edge.to;
        else if (edge.to == current) current = edge.from;
        else throw std::invalid_argument("Edge path is not contiguous.");
        result.push_back(current);
    }
    return result;
}

WeightedGraph WeightedGraph::makeDemoNetwork() {
    WeightedGraph graph;
    constexpr uint32_t columns = 10U;
    constexpr uint32_t rows = 7U;
    for (uint32_t y = 0U; y < rows; ++y) {
        for (uint32_t x = 0U; x < columns; ++x) {
            const float jitterX = static_cast<float>((x * 17U + y * 11U) % 7U) * 0.003F - 0.009F;
            const float jitterY = static_cast<float>((x * 13U + y * 19U) % 7U) * 0.003F - 0.009F;
            graph.addNode((static_cast<float>(x) + 0.5F) / static_cast<float>(columns) + jitterX,
                          (static_cast<float>(y) + 0.5F) / static_cast<float>(rows) + jitterY);
        }
    }
    const auto node = [](const uint32_t x, const uint32_t y) { return y * columns + x; };
    const auto edgeCost = [](const uint32_t from, const uint32_t to, const float base) {
        const float variation = 0.82F + static_cast<float>((from * 31U + to * 17U) % 13U) * 0.035F;
        return base * variation;
    };
    for (uint32_t y = 0U; y < rows; ++y) {
        for (uint32_t x = 0U; x + 1U < columns; ++x) {
            graph.addUndirectedEdge(node(x, y), node(x + 1U, y),
                                    edgeCost(node(x, y), node(x + 1U, y), 5.0F));
        }
    }
    for (uint32_t y = 0U; y + 1U < rows; ++y) {
        for (uint32_t x = 0U; x < columns; ++x) {
            graph.addUndirectedEdge(node(x, y), node(x, y + 1U),
                                    edgeCost(node(x, y), node(x, y + 1U), 5.6F));
        }
    }
    for (uint32_t y = 0U; y + 1U < rows; ++y) {
        for (uint32_t x = 0U; x + 1U < columns; ++x) {
            if (((x + y) & 1U) == 0U) {
                graph.addUndirectedEdge(node(x, y), node(x + 1U, y + 1U),
                                        edgeCost(node(x, y), node(x + 1U, y + 1U), 7.0F));
            } else {
                graph.addUndirectedEdge(node(x + 1U, y), node(x, y + 1U),
                                        edgeCost(node(x + 1U, y), node(x, y + 1U), 7.0F));
            }
        }
    }
    return graph;
}

WeightedGraph WeightedGraph::makeGridNetwork(const comparison::Scenario& scenario) {
    scenario.validate();
    if (scenario.nodeCount() > 1'000'000U) {
        throw std::invalid_argument("Materialized graphs are limited to one million cells; use the implicit GPU grid.");
    }
    WeightedGraph graph;
    for (uint32_t y = 0U; y < scenario.height; ++y) {
        for (uint32_t x = 0U; x < scenario.width; ++x) {
            graph.addNode(
                (static_cast<float>(x) + 0.5F) / static_cast<float>(scenario.width),
                (static_cast<float>(y) + 0.5F) / static_cast<float>(scenario.height));
        }
    }
    for (uint32_t y = 0U; y < scenario.height; ++y) {
        for (uint32_t x = 0U; x < scenario.width; ++x) {
            const uint32_t current = y * scenario.width + x;
            if (!scenario.traversable(current)) continue;
            if (x + 1U < scenario.width && scenario.traversable(current + 1U)) {
                graph.addUndirectedEdge(current, current + 1U, 1.0F);
            }
            if (y + 1U < scenario.height && scenario.traversable(current + scenario.width)) {
                graph.addUndirectedEdge(current, current + scenario.width, 1.0F);
            }
        }
    }
    if (graph.edges().empty()) throw std::invalid_argument("Comparison grid has no traversable edges.");
    return graph;
}
