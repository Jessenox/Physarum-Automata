#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace comparison { struct Scenario; }

struct GraphNode {
    float x = 0.0F;
    float y = 0.0F;
};

struct GraphEdge {
    uint32_t from = 0U;
    uint32_t to = 0U;
    float cost = 1.0F;
};

struct GraphAdjacency {
    uint32_t neighbor = 0U;
    uint32_t edge = 0U;
};

class WeightedGraph {
public:
    uint32_t addNode(float x, float y);
    uint32_t addUndirectedEdge(uint32_t from, uint32_t to, float cost);

    [[nodiscard]] const std::vector<GraphNode>& nodes() const;
    [[nodiscard]] const std::vector<GraphEdge>& edges() const;
    [[nodiscard]] const std::vector<GraphAdjacency>& neighbors(uint32_t node) const;
    [[nodiscard]] float pathCost(uint32_t start, std::span<const uint32_t> edgePath) const;
    [[nodiscard]] std::vector<uint32_t> nodePath(uint32_t start, std::span<const uint32_t> edgePath) const;

    [[nodiscard]] static WeightedGraph makeDemoNetwork();
    [[nodiscard]] static WeightedGraph makeGridNetwork(const comparison::Scenario& scenario);

private:
    std::vector<GraphNode> nodes_;
    std::vector<GraphEdge> edges_;
    std::vector<std::vector<GraphAdjacency>> adjacency_;
};
