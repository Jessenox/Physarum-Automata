#include "domain/GridModel.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <random>
#include <stdexcept>
#include <utility>

namespace {

constexpr uint32_t kInfinity = std::numeric_limits<uint32_t>::max();

std::size_t checkedNodeCount(const GridSize size) {
    if (size.width < 2 || size.height < 2) {
        throw std::invalid_argument("Grid dimensions must both be at least 2.");
    }
    const uint64_t count = static_cast<uint64_t>(size.width) * size.height;
    if (count >= kInfinity) {
        throw std::invalid_argument("Grid exceeds the 32-bit node index space.");
    }
    return static_cast<std::size_t>(count);
}

}

GridModel::GridModel(const GridSize size)
    : size_(size),
      weights_(checkedNodeCount(size), 1U) {
    source_ = (size.height / 2U) * size.width + size.width / 5U;
    target_ = (size.height / 2U) * size.width + (size.width * 4U) / 5U;
}

GridSize GridModel::size() const {
    return size_;
}

uint32_t GridModel::nodeCount() const {
    return static_cast<uint32_t>(static_cast<uint64_t>(size_.width) * size_.height);
}

uint32_t GridModel::source() const {
    return source_;
}

uint32_t GridModel::target() const {
    return target_;
}

const std::vector<uint8_t>& GridModel::weights() const {
    return weights_;
}

uint8_t GridModel::weight(const uint32_t index) const {
    return weights_.at(index);
}

void GridModel::setSource(const uint32_t index) {
    if (!validIndex(index) || index == target_) {
        return;
    }
    source_ = index;
    weights_[index] = std::max<uint8_t>(1U, weights_[index]);
}

void GridModel::setTarget(const uint32_t index) {
    if (!validIndex(index) || index == source_) {
        return;
    }
    target_ = index;
    weights_[index] = std::max<uint8_t>(1U, weights_[index]);
}

void GridModel::setWeight(const uint32_t index, const uint32_t weight) {
    if (!validIndex(index) || index == source_ || index == target_) {
        return;
    }
    if (weight != 0U && weight != 1U && weight != 2U && weight != 5U) {
        throw std::invalid_argument("Cell weight must be 0, 1, 2, or 5.");
    }
    weights_[index] = static_cast<uint8_t>(weight);
}

void GridModel::clearWalls() {
    std::fill(weights_.begin(), weights_.end(), 1U);
}

void GridModel::randomizeWalls(const uint32_t seed, const float density) {
    std::mt19937 generator(seed);
    std::uniform_real_distribution<float> probability(0.0f, 1.0f);
    for (uint32_t index = 0; index < nodeCount(); ++index) {
        if (index == source_ || index == target_) {
            weights_[index] = 1U;
            continue;
        }
        weights_[index] = probability(generator) < density ? 0U : 1U;
    }
}

DijkstraResult GridModel::solveCpu() const {
    using QueueEntry = std::pair<uint32_t, uint32_t>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> queue;
    std::vector<uint32_t> distances(nodeCount(), kInfinity);
    std::vector<uint32_t> previous(nodeCount(), kInfinity);
    std::vector<uint8_t> visited(nodeCount(), 0U);

    distances[source_] = 0U;
    queue.emplace(0U, source_);
    uint32_t visitedNodes = 0U;

    while (!queue.empty()) {
        const auto [distance, current] = queue.top();
        queue.pop();
        if (visited[current] != 0U || distance != distances[current]) {
            continue;
        }

        visited[current] = 1U;
        ++visitedNodes;
        if (current == target_) {
            break;
        }

        const uint32_t x = current % size_.width;
        const uint32_t y = current / size_.width;
        const uint32_t candidates[4] = {
            x > 0U ? current - 1U : kInfinity,
            x + 1U < size_.width ? current + 1U : kInfinity,
            y > 0U ? current - size_.width : kInfinity,
            y + 1U < size_.height ? current + size_.width : kInfinity,
        };

        for (const uint32_t next : candidates) {
            if (next == kInfinity || weights_[next] == 0U || visited[next] != 0U) {
                continue;
            }
            const uint32_t candidateDistance = distance + weights_[next];
            if (candidateDistance < distances[next]) {
                distances[next] = candidateDistance;
                previous[next] = current;
                queue.emplace(candidateDistance, next);
            }
        }
    }

    DijkstraResult result{};
    result.found = distances[target_] != kInfinity;
    result.distance = result.found ? distances[target_] : 0U;
    result.visitedNodes = visitedNodes;
    if (!result.found) {
        return result;
    }

    for (uint32_t current = target_;; current = previous[current]) {
        result.path.push_back(current);
        if (current == source_) {
            break;
        }
        if (previous[current] == kInfinity) {
            throw std::runtime_error("Dijkstra predecessor chain is incomplete.");
        }
    }
    std::reverse(result.path.begin(), result.path.end());
    return result;
}

bool GridModel::validIndex(const uint32_t index) const {
    return index < nodeCount();
}
