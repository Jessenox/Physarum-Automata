#pragma once

#include "domain/Graph.h"

#include <cstdint>
#include <random>
#include <span>
#include <vector>

struct ACOConfig {
    uint32_t ants = 30U;
    uint32_t iterations = 300U;
    float alpha = 1.0F;
    float beta = 2.0F;
    float rho = 0.10F;
    float depositQ = 100.0F;
    float initialPheromone = 1.0F;
    uint32_t seed = 12345U;
    uint32_t stagnationLimit = 0U;
};

struct AntRoute {
    uint32_t antId = 0U;
    std::vector<uint32_t> nodes;
    std::vector<uint32_t> edges;
    float cost = 0.0F;
    bool reachedGoal = false;
};

class PheromoneState {
public:
    PheromoneState(uint32_t edgeCount, uint32_t antCount, float initialPheromone);

    [[nodiscard]] float total(uint32_t edge) const;
    [[nodiscard]] float contribution(uint32_t edge, uint32_t ant) const;
    void evaporate(float rho);
    void deposit(uint32_t ant, std::span<const uint32_t> edgePath, float amount);

    [[nodiscard]] const std::vector<float>& base() const;
    [[nodiscard]] const std::vector<float>& byAnt() const;

private:
    uint32_t edgeCount_ = 0U;
    uint32_t antCount_ = 0U;
    std::vector<float> base_;
    std::vector<float> byAnt_;
};

struct InfluenceMetrics {
    std::vector<float> outStrength;
    std::vector<float> inStrength;
    uint32_t mostInfluentialAnt = 0U;
    float maxOutStrength = 0.0F;
    float concentration = 0.0F;
    float entropy = 0.0F;
};

class InteractionNetwork {
public:
    explicit InteractionNetwork(uint32_t antCount);

    void beginIteration();
    void recordTraversal(uint32_t targetAnt, uint32_t edge, const PheromoneState& pheromone);
    void endIteration();

    [[nodiscard]] InfluenceMetrics metrics() const;
    [[nodiscard]] const std::vector<float>& current() const;
    [[nodiscard]] const std::vector<float>& total() const;

private:
    uint32_t antCount_ = 0U;
    std::vector<float> current_;
    std::vector<float> total_;
};

struct IterationStats {
    uint32_t iteration = 0U;
    float bestCost = 0.0F;
    float averageCost = 0.0F;
    uint32_t successfulAnts = 0U;
    uint32_t bestAnt = 0U;
    InfluenceMetrics influence;
    std::vector<float> interaction;
};

struct ACOResult {
    std::vector<uint32_t> bestNodes;
    std::vector<uint32_t> bestEdges;
    float bestCost = 0.0F;
    uint32_t foundAtIteration = 0U;
    uint32_t iterations = 0U;
    std::vector<float> interactionTotal;
    std::vector<IterationStats> history;
};

class ACOEngine {
public:
    ACOEngine(WeightedGraph graph, uint32_t start, uint32_t goal, ACOConfig config = {});

    [[nodiscard]] ACOResult run();
    [[nodiscard]] static std::vector<float> selectionProbabilities(
        const WeightedGraph& graph, const PheromoneState& pheromone,
        std::span<const GraphAdjacency> candidates, float alpha, float beta);

private:
    [[nodiscard]] AntRoute constructRoute(uint32_t antId);

    WeightedGraph graph_;
    uint32_t start_ = 0U;
    uint32_t goal_ = 0U;
    ACOConfig config_{};
    PheromoneState pheromone_;
    InteractionNetwork interaction_;
    std::mt19937 random_;
};
