#include "domain/NetworkACO.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

PheromoneState::PheromoneState(const uint32_t edgeCount, const uint32_t antCount,
                               const float initialPheromone)
    : edgeCount_(edgeCount), antCount_(antCount), base_(edgeCount, initialPheromone),
      byAnt_(static_cast<std::size_t>(edgeCount) * antCount, 0.0F) {
    if (edgeCount == 0U || antCount == 0U || initialPheromone <= 0.0F) {
        throw std::invalid_argument("Invalid pheromone state dimensions or initial value.");
    }
}

float PheromoneState::total(const uint32_t edge) const {
    float value = base_.at(edge);
    const std::size_t offset = static_cast<std::size_t>(edge) * antCount_;
    for (uint32_t ant = 0U; ant < antCount_; ++ant) value += byAnt_[offset + ant];
    return value;
}

float PheromoneState::contribution(const uint32_t edge, const uint32_t ant) const {
    return byAnt_.at(static_cast<std::size_t>(edge) * antCount_ + ant);
}

void PheromoneState::evaporate(const float rho) {
    if (rho < 0.0F || rho >= 1.0F) throw std::invalid_argument("rho must be in [0, 1).");
    const float remaining = 1.0F - rho;
    for (float& value : base_) value *= remaining;
    for (float& value : byAnt_) value *= remaining;
}

void PheromoneState::deposit(const uint32_t ant, const std::span<const uint32_t> edgePath,
                             const float amount) {
    if (ant >= antCount_ || amount < 0.0F) throw std::invalid_argument("Invalid pheromone deposit.");
    for (const uint32_t edge : edgePath) {
        byAnt_.at(static_cast<std::size_t>(edge) * antCount_ + ant) += amount;
    }
}

const std::vector<float>& PheromoneState::base() const { return base_; }
const std::vector<float>& PheromoneState::byAnt() const { return byAnt_; }

InteractionNetwork::InteractionNetwork(const uint32_t antCount)
    : antCount_(antCount), current_(static_cast<std::size_t>(antCount) * antCount, 0.0F),
      total_(static_cast<std::size_t>(antCount) * antCount, 0.0F) {
    if (antCount == 0U) throw std::invalid_argument("Interaction network needs ants.");
}

void InteractionNetwork::beginIteration() { std::fill(current_.begin(), current_.end(), 0.0F); }

void InteractionNetwork::recordTraversal(const uint32_t targetAnt, const uint32_t edge,
                                         const PheromoneState& pheromone) {
    if (targetAnt >= antCount_) throw std::out_of_range("Invalid target ant.");
    for (uint32_t sourceAnt = 0U; sourceAnt < antCount_; ++sourceAnt) {
        if (sourceAnt == targetAnt) continue;
        current_[static_cast<std::size_t>(sourceAnt) * antCount_ + targetAnt]
            += pheromone.contribution(edge, sourceAnt);
    }
}

void InteractionNetwork::endIteration() {
    for (std::size_t index = 0U; index < total_.size(); ++index) total_[index] += current_[index];
}

InfluenceMetrics InteractionNetwork::metrics() const {
    InfluenceMetrics result;
    result.outStrength.assign(antCount_, 0.0F);
    result.inStrength.assign(antCount_, 0.0F);
    for (uint32_t source = 0U; source < antCount_; ++source) {
        for (uint32_t target = 0U; target < antCount_; ++target) {
            const float value = current_[static_cast<std::size_t>(source) * antCount_ + target];
            result.outStrength[source] += value;
            result.inStrength[target] += value;
        }
    }
    const auto strongest = std::max_element(result.outStrength.begin(), result.outStrength.end());
    result.mostInfluentialAnt = static_cast<uint32_t>(strongest - result.outStrength.begin());
    result.maxOutStrength = *strongest;
    const float sum = std::accumulate(result.outStrength.begin(), result.outStrength.end(), 0.0F);
    if (sum > 0.0F) {
        result.concentration = result.maxOutStrength / sum;
        for (const float strength : result.outStrength) {
            if (strength <= 0.0F) continue;
            const float probability = strength / sum;
            result.entropy -= probability * std::log(probability);
        }
    }
    return result;
}

const std::vector<float>& InteractionNetwork::current() const { return current_; }
const std::vector<float>& InteractionNetwork::total() const { return total_; }

ACOEngine::ACOEngine(WeightedGraph graph, const uint32_t start, const uint32_t goal, ACOConfig config)
    : graph_(std::move(graph)), start_(start), goal_(goal), config_(config),
      pheromone_(static_cast<uint32_t>(graph_.edges().size()), config.ants, config.initialPheromone),
      interaction_(config.ants), random_(config.seed) {
    if (start >= graph_.nodes().size() || goal >= graph_.nodes().size() || start == goal) {
        throw std::invalid_argument("Invalid ACO endpoints.");
    }
    if (config_.ants < 2U || config_.iterations == 0U || config_.alpha < 0.0F || config_.beta < 0.0F
        || config_.depositQ <= 0.0F || config_.rho < 0.0F || config_.rho >= 1.0F) {
        throw std::invalid_argument("Invalid ACO configuration.");
    }
}

std::vector<float> ACOEngine::selectionProbabilities(
    const WeightedGraph& graph, const PheromoneState& pheromone,
    const std::span<const GraphAdjacency> candidates, const float alpha, const float beta) {
    std::vector<float> result;
    result.reserve(candidates.size());
    float totalWeight = 0.0F;
    for (const GraphAdjacency& candidate : candidates) {
        const float tau = std::max(pheromone.total(candidate.edge), std::numeric_limits<float>::min());
        const float eta = 1.0F / graph.edges().at(candidate.edge).cost;
        const float weight = std::pow(tau, alpha) * std::pow(eta, beta);
        result.push_back(weight);
        totalWeight += weight;
    }
    if (result.empty()) return result;
    if (!std::isfinite(totalWeight) || totalWeight <= 0.0F) {
        std::fill(result.begin(), result.end(), 1.0F / static_cast<float>(result.size()));
    } else {
        for (float& value : result) value /= totalWeight;
    }
    return result;
}

AntRoute ACOEngine::constructRoute(const uint32_t antId) {
    AntRoute route;
    route.antId = antId;
    route.nodes.push_back(start_);
    std::vector<bool> visited(graph_.nodes().size(), false);
    visited[start_] = true;
    uint32_t current = start_;
    std::uniform_real_distribution<float> unit(0.0F, 1.0F);
    while (current != goal_) {
        std::vector<GraphAdjacency> candidates;
        for (const GraphAdjacency adjacency : graph_.neighbors(current)) {
            if (!visited[adjacency.neighbor]) candidates.push_back(adjacency);
        }
        if (candidates.empty()) return route;
        const std::vector<float> probabilities = selectionProbabilities(
            graph_, pheromone_, candidates, config_.alpha, config_.beta);
        const float draw = unit(random_);
        float cumulative = 0.0F;
        std::size_t chosen = candidates.size() - 1U;
        for (std::size_t index = 0U; index < candidates.size(); ++index) {
            cumulative += probabilities[index];
            if (draw <= cumulative) {
                chosen = index;
                break;
            }
        }
        const GraphAdjacency next = candidates[chosen];
        interaction_.recordTraversal(antId, next.edge, pheromone_);
        route.edges.push_back(next.edge);
        route.cost += graph_.edges()[next.edge].cost;
        current = next.neighbor;
        route.nodes.push_back(current);
        visited[current] = true;
    }
    route.reachedGoal = true;
    return route;
}

ACOResult ACOEngine::run() {
    ACOResult result;
    result.bestCost = std::numeric_limits<float>::infinity();
    uint32_t stagnant = 0U;
    for (uint32_t iteration = 0U; iteration < config_.iterations; ++iteration) {
        interaction_.beginIteration();
        std::vector<AntRoute> routes;
        routes.reserve(config_.ants);
        float costSum = 0.0F;
        uint32_t successful = 0U;
        float iterationBest = std::numeric_limits<float>::infinity();
        uint32_t bestAnt = 0U;
        bool improved = false;
        for (uint32_t ant = 0U; ant < config_.ants; ++ant) {
            routes.push_back(constructRoute(ant));
            const AntRoute& route = routes.back();
            if (!route.reachedGoal) continue;
            ++successful;
            costSum += route.cost;
            if (route.cost < iterationBest) {
                iterationBest = route.cost;
                bestAnt = ant;
            }
            if (route.cost < result.bestCost) {
                result.bestCost = route.cost;
                result.bestNodes = route.nodes;
                result.bestEdges = route.edges;
                result.foundAtIteration = iteration + 1U;
                improved = true;
            }
        }
        interaction_.endIteration();
        pheromone_.evaporate(config_.rho);
        for (const AntRoute& route : routes) {
            if (route.reachedGoal && route.cost > 0.0F) {
                pheromone_.deposit(route.antId, route.edges, config_.depositQ / route.cost);
            }
        }
        IterationStats stats;
        stats.iteration = iteration + 1U;
        stats.bestCost = iterationBest;
        stats.averageCost = successful > 0U ? costSum / static_cast<float>(successful)
                                            : std::numeric_limits<float>::infinity();
        stats.successfulAnts = successful;
        stats.bestAnt = bestAnt;
        stats.influence = interaction_.metrics();
        stats.interaction = interaction_.current();
        result.history.push_back(std::move(stats));
        result.iterations = iteration + 1U;
        if (improved) stagnant = 0U;
        else ++stagnant;
        if (config_.stagnationLimit > 0U && stagnant >= config_.stagnationLimit) break;
    }
    result.interactionTotal = interaction_.total();
    return result;
}
