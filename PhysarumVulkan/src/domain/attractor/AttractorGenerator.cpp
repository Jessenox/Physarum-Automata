#include "domain/attractor/AttractorGenerator.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace {

constexpr uint8_t kVisibleStatesPerCell = 9U;
constexpr uint8_t kCombinedStatesPerCell = 81U;
constexpr float kCycleRadius = 28.0f;
constexpr float kTreeLayerGap = 34.0f;
constexpr float kComponentGap = 96.0f;
constexpr float kMinComponentExtent = 72.0f;
constexpr float kRootClusterOverlap = 0.58f;
constexpr float kMinBranchRadius = 26.0f;
constexpr float kPi = 3.14159265358979323846f;
constexpr std::size_t kSeedBatchSize = 2048U;

struct AttractorStateBlockHash {
    std::size_t operator()(const AttractorStateBlock& state) const noexcept {
        uint64_t hash = 0x9e3779b97f4a7c15ULL;
        for (std::size_t index = 0; index < state.cells.size(); ++index) {
            hash ^= static_cast<uint64_t>(state.cells[index] + 1U) * (0x100000001b3ULL + static_cast<uint64_t>(index) * 0x9e37ULL);
            hash ^= hash >> 33U;
            hash *= 0xff51afd7ed558ccdULL;
            hash ^= hash >> 29U;
        }
        return static_cast<std::size_t>(hash);
    }
};

struct WorkingNode {
    AttractorStateBlock key{};
    uint32_t next = UINT32_MAX;
    uint32_t visits = 0;
    std::vector<uint32_t> incoming;
    bool cycle = false;
    float x = 0.0f;
    float y = 0.0f;
};

struct SeedWalker {
    AttractorStateBlock currentKey{};
    bool finished = false;
    std::vector<AttractorStateBlock> localPath;
    std::unordered_map<AttractorStateBlock, std::size_t, AttractorStateBlockHash> localSeen;
};

using ProgressCallback = std::function<void(const AttractorProgress&)>;
using GraphCallback = std::function<void(AttractorGraph)>;

uint64_t powChecked(const uint64_t base, const uint32_t exponent) {
    uint64_t result = 1ULL;
    for (uint32_t index = 0; index < exponent; ++index) {
        if (result > std::numeric_limits<uint64_t>::max() / base) {
            throw std::runtime_error("State space exceeds uint64_t.");
        }
        result *= base;
    }
    return result;
}

std::size_t cellCount(const AttractorSettings& settings) {
    return static_cast<std::size_t>(settings.width) * static_cast<std::size_t>(settings.height);
}

bool usesApproximateSampling(const AttractorSettings& settings) {
    return cellCount(settings) > 9U;
}

std::size_t fullWidth(const AttractorSettings& settings) {
    return static_cast<std::size_t>(settings.width) + 2U;
}

std::size_t fullHeight(const AttractorSettings& settings) {
    return static_cast<std::size_t>(settings.height) + 2U;
}

std::size_t fullIndex(const AttractorSettings& settings, const uint32_t x, const uint32_t y) {
    return static_cast<std::size_t>(y) * fullWidth(settings) + static_cast<std::size_t>(x);
}

uint64_t mix64(uint64_t value) {
    value ^= value >> 33U;
    value *= 0xff51afd7ed558ccdULL;
    value ^= value >> 33U;
    value *= 0xc4ceb9fe1a85ec53ULL;
    value ^= value >> 33U;
    return value;
}

uint64_t hashStateBlock(const AttractorStateBlock& state, const std::size_t activeCells) {
    uint64_t hash = 0x84222325cbf29ce4ULL ^ static_cast<uint64_t>(activeCells);
    for (std::size_t index = 0; index < activeCells; ++index) {
        hash ^= static_cast<uint64_t>(state.cells[index] + 1U) * 0x100000001b3ULL;
        hash = mix64(hash + static_cast<uint64_t>(index + 1U) * 0x9e3779b97f4a7c15ULL);
    }
    return hash;
}

uint64_t directionSeedForState(const AttractorStateBlock& state, const std::size_t activeCells) {
    uint64_t encoded = 0ULL;
    uint64_t base = 1ULL;
    for (std::size_t index = 0; index < activeCells; ++index) {
        if (base > std::numeric_limits<uint64_t>::max() / static_cast<uint64_t>(kCombinedStatesPerCell)) {
            return hashStateBlock(state, activeCells);
        }
        encoded += static_cast<uint64_t>(state.cells[index]) * base;
        base *= static_cast<uint64_t>(kCombinedStatesPerCell);
    }
    return encoded;
}

uint8_t deterministicDirection(
    const uint64_t stateSeed,
    const uint32_t x,
    const uint32_t y,
    const uint8_t currentCell,
    const uint8_t currentMemory) {
    uint64_t value = stateSeed;
    value ^= static_cast<uint64_t>(x + 1U) * 0x9e3779b97f4a7c15ULL;
    value ^= static_cast<uint64_t>(y + 1U) * 0xc2b2ae3d27d4eb4fULL;
    value ^= static_cast<uint64_t>(currentCell + 1U) * 0x165667b19e3779f9ULL;
    value ^= static_cast<uint64_t>(currentMemory + 1U) * 0x85ebca77c2b2ae63ULL;
    return static_cast<uint8_t>(mix64(value) % 8ULL);
}

AttractorStateBlock encodeSeedState(const uint64_t seed, const AttractorSettings& settings) {
    AttractorStateBlock encoded{};
    uint64_t remaining = seed;
    const std::size_t totalCells = cellCount(settings);
    for (std::size_t index = 0; index < totalCells; ++index) {
        encoded.cells[index] = static_cast<uint8_t>(remaining % static_cast<uint64_t>(kVisibleStatesPerCell));
        remaining /= static_cast<uint64_t>(kVisibleStatesPerCell);
    }
    return encoded;
}

AttractorStateBlock sampleApproximateSeed(
    const AttractorSettings& settings,
    const uint64_t seedBase,
    const uint64_t sampleIndex) {
    AttractorStateBlock encoded{};
    const std::size_t totalCells = cellCount(settings);
    uint64_t value = mix64(seedBase ^ (sampleIndex + 1ULL) * 0x9e3779b97f4a7c15ULL);
    for (std::size_t index = 0; index < totalCells; ++index) {
        value = mix64(value + 0x94d049bb133111ebULL + static_cast<uint64_t>(index + 1U) * 0xbf58476d1ce4e5b9ULL);
        encoded.cells[index] = static_cast<uint8_t>(value % static_cast<uint64_t>(kVisibleStatesPerCell));
    }
    return encoded;
}

void decodeStateBlock(
    const AttractorStateBlock& block,
    const AttractorSettings& settings,
    std::vector<uint8_t>& states,
    std::vector<uint8_t>& memory) {
    states.assign(fullWidth(settings) * fullHeight(settings), 2U);
    memory.assign(fullWidth(settings) * fullHeight(settings), 0U);

    std::size_t cellIndex = 0U;
    for (uint32_t y = 1; y <= settings.height; ++y) {
        for (uint32_t x = 1; x <= settings.width; ++x) {
            const uint8_t digit = block.cells[cellIndex++];
            states[fullIndex(settings, x, y)] = static_cast<uint8_t>(digit % kVisibleStatesPerCell);
            memory[fullIndex(settings, x, y)] = static_cast<uint8_t>(digit / kVisibleStatesPerCell);
        }
    }
}

AttractorStateBlock encodeCombinedState(
    const AttractorSettings& settings,
    const std::vector<uint8_t>& states,
    const std::vector<uint8_t>& memory) {
    AttractorStateBlock block{};
    std::size_t cellIndex = 0U;
    for (uint32_t y = 1; y <= settings.height; ++y) {
        for (uint32_t x = 1; x <= settings.width; ++x) {
            const std::size_t index = fullIndex(settings, x, y);
            block.cells[cellIndex++] = static_cast<uint8_t>(
                states[index] + memory[index] * static_cast<uint8_t>(kVisibleStatesPerCell));
        }
    }
    return block;
}

void gatherNeighbours(
    const AttractorSettings& settings,
    const std::vector<uint8_t>& matrix,
    const uint32_t x,
    const uint32_t y,
    std::array<uint16_t, 8>& out) {
    static constexpr std::array<std::pair<int, int>, 8> kOffsets{{
        {-1, 0},
        {-1, 1},
        {0, 1},
        {1, 1},
        {1, 0},
        {1, -1},
        {0, -1},
        {-1, -1},
    }};

    for (std::size_t index = 0; index < kOffsets.size(); ++index) {
        const auto [deltaX, deltaY] = kOffsets[index];
        const uint32_t sampleX = static_cast<uint32_t>(static_cast<int>(x) + deltaX);
        const uint32_t sampleY = static_cast<uint32_t>(static_cast<int>(y) + deltaY);
        out[index] = matrix[fullIndex(settings, sampleX, sampleY)];
    }
}

void applyCornerWalls(std::array<uint16_t, 8>& neighboursData) {
    if (neighboursData[0] == 2 && neighboursData[2] == 2) {
        neighboursData[1] = 2;
    }
    if (neighboursData[0] == 2 && neighboursData[6] == 2) {
        neighboursData[7] = 2;
    }
    if (neighboursData[6] == 2 && neighboursData[4] == 2) {
        neighboursData[5] = 2;
    }
    if (neighboursData[2] == 2 && neighboursData[4] == 2) {
        neighboursData[3] = 2;
    }
}

bool hasNeighbourState(
    const std::array<uint16_t, 8>& neighboursData,
    const int direction,
    const uint16_t state) {
    return neighboursData.at(static_cast<std::size_t>(direction)) == state;
}

bool isOnMooreOffset(const std::array<uint16_t, 8>& neighboursDirections) {
    for (std::size_t index = 4, expected = 0; expected < neighboursDirections.size(); ++index, ++expected) {
        if (index > 7) {
            index = 0;
        }
        if (neighboursDirections[index] > 0 &&
            static_cast<std::size_t>(neighboursDirections[index] - 1U) == expected) {
            return true;
        }
    }
    return false;
}

void findStates(
    const std::array<uint16_t, 8>& neighboursData,
    std::array<bool, 9>& statesFound) {
    statesFound.fill(false);
    for (const uint16_t value : neighboursData) {
        if (value < statesFound.size()) {
            statesFound[value] = true;
        }
    }
}

AttractorStateBlock evaluateDeterministicSuccessor(
    const AttractorStateBlock& currentKey,
    const AttractorSettings& settings,
    std::vector<uint8_t>& states,
    std::vector<uint8_t>& auxStates,
    std::vector<uint8_t>& memory) {
    decodeStateBlock(currentKey, settings, states, memory);
    auxStates = states;

    const uint64_t stateSeed = directionSeedForState(currentKey, cellCount(settings));
    std::array<uint16_t, 8> neighboursData{};
    std::array<uint16_t, 8> neighboursDirections{};
    std::array<bool, 9> statesFound{};

    for (uint32_t y = 1; y <= settings.height; ++y) {
        for (uint32_t x = 1; x <= settings.width; ++x) {
            gatherNeighbours(settings, states, x, y, neighboursData);
            gatherNeighbours(settings, memory, x, y, neighboursDirections);
            applyCornerWalls(neighboursData);
            findStates(neighboursData, statesFound);

            const std::size_t index = fullIndex(settings, x, y);
            const uint8_t currentCell = states[index];
            const uint8_t currentMemory = memory[index];
            const int currentDirection =
                static_cast<int>(deterministicDirection(stateSeed, x - 1U, y - 1U, currentCell, currentMemory));

            const auto writeAux = [&](const uint8_t newState) {
                auxStates[index] = newState;
            };

            switch (currentCell) {
                case 0:
                    if ((hasNeighbourState(neighboursData, currentDirection, 3) ||
                         hasNeighbourState(neighboursData, currentDirection, 4) ||
                         hasNeighbourState(neighboursData, currentDirection, 6)) &&
                        memory[index] == 0) {
                        writeAux(7);
                    }
                    break;
                case 1:
                    if (statesFound[5] || statesFound[6]) {
                        writeAux(6);
                    }
                    break;
                case 2:
                case 3:
                case 6:
                    break;
                case 4:
                    if ((hasNeighbourState(neighboursData, currentDirection, 3) ||
                         hasNeighbourState(neighboursData, currentDirection, 5) ||
                         hasNeighbourState(neighboursData, currentDirection, 6)) &&
                        memory[index] == 0 &&
                        !statesFound[0] &&
                        !statesFound[7]) {
                        writeAux(5);
                        memory[index] = static_cast<uint8_t>(currentDirection + 1);
                    }
                    break;
                case 5:
                    if (!isOnMooreOffset(neighboursDirections) &&
                        !isOnMooreOffset(neighboursDirections) &&
                        !statesFound[1] &&
                        !statesFound[3] &&
                        !statesFound[4] &&
                        !statesFound[6]) {
                        writeAux(0);
                        memory[index] = 0;
                    } else {
                        writeAux(8);
                    }
                    break;
                case 7:
                    if (statesFound[3] || statesFound[4] || statesFound[6]) {
                        writeAux(4);
                    }
                    break;
                case 8:
                    writeAux(5);
                    break;
                default:
                    break;
            }
        }
    }

    return encodeCombinedState(settings, auxStates, memory);
}

void evaluateSuccessorBatchCpu(
    const AttractorSettings& settings,
    const std::vector<AttractorStateBlock>& inputs,
    std::vector<AttractorStateBlock>& outputs) {
    outputs.resize(inputs.size());

    std::vector<uint8_t> states;
    std::vector<uint8_t> auxStates;
    std::vector<uint8_t> memory;
    for (std::size_t index = 0; index < inputs.size(); ++index) {
        outputs[index] = evaluateDeterministicSuccessor(inputs[index], settings, states, auxStates, memory);
    }
}

uint32_t addNode(
    const AttractorStateBlock& key,
    std::unordered_map<AttractorStateBlock, uint32_t, AttractorStateBlockHash>& keyToNode,
    std::vector<WorkingNode>& nodes) {
    const auto found = keyToNode.find(key);
    if (found != keyToNode.end()) {
        return found->second;
    }

    const uint32_t nodeIndex = static_cast<uint32_t>(nodes.size());
    keyToNode.emplace(key, nodeIndex);
    WorkingNode node{};
    node.key = key;
    nodes.push_back(std::move(node));
    return nodeIndex;
}

void connectLocalPathToExisting(
    const std::vector<AttractorStateBlock>& localPath,
    const uint32_t destinationNode,
    std::unordered_map<AttractorStateBlock, uint32_t, AttractorStateBlockHash>& keyToNode,
    std::vector<WorkingNode>& nodes) {
    std::vector<uint32_t> indices(localPath.size(), UINT32_MAX);
    for (std::size_t index = 0; index < localPath.size(); ++index) {
        indices[index] = addNode(localPath[index], keyToNode, nodes);
        ++nodes[indices[index]].visits;
    }

    for (std::size_t index = 0; index < indices.size(); ++index) {
        nodes[indices[index]].next =
            (index + 1U < indices.size()) ? indices[index + 1U] : destinationNode;
    }
}

void connectLocalCycle(
    const std::vector<AttractorStateBlock>& localPath,
    const std::size_t cycleStart,
    std::unordered_map<AttractorStateBlock, uint32_t, AttractorStateBlockHash>& keyToNode,
    std::vector<WorkingNode>& nodes) {
    std::vector<uint32_t> indices(localPath.size(), UINT32_MAX);
    for (std::size_t index = 0; index < localPath.size(); ++index) {
        indices[index] = addNode(localPath[index], keyToNode, nodes);
        ++nodes[indices[index]].visits;
    }

    for (std::size_t index = 0; index < cycleStart; ++index) {
        nodes[indices[index]].next = indices[index + 1U];
    }

    for (std::size_t index = cycleStart; index + 1U < indices.size(); ++index) {
        nodes[indices[index]].next = indices[index + 1U];
    }
    nodes[indices.back()].next = indices[cycleStart];
}

void markCycles(std::vector<WorkingNode>& nodes) {
    std::vector<uint32_t> indegree(nodes.size(), 0U);
    for (const WorkingNode& node : nodes) {
        if (node.next != UINT32_MAX) {
            ++indegree[node.next];
        }
    }

    std::vector<uint32_t> stack;
    stack.reserve(nodes.size());
    for (uint32_t index = 0; index < indegree.size(); ++index) {
        if (indegree[index] == 0U) {
            stack.push_back(index);
        }
    }

    while (!stack.empty()) {
        const uint32_t nodeIndex = stack.back();
        stack.pop_back();
        const uint32_t next = nodes[nodeIndex].next;
        if (next == UINT32_MAX) {
            continue;
        }
        if (--indegree[next] == 0U) {
            stack.push_back(next);
        }
    }

    for (uint32_t index = 0; index < indegree.size(); ++index) {
        nodes[index].cycle = indegree[index] > 0U;
    }
}

void buildIncomingLists(std::vector<WorkingNode>& nodes) {
    for (WorkingNode& node : nodes) {
        node.incoming.clear();
    }
    for (uint32_t index = 0; index < nodes.size(); ++index) {
        const uint32_t next = nodes[index].next;
        if (next != UINT32_MAX) {
            nodes[next].incoming.push_back(index);
        }
    }
}

uint32_t subtreeWeight(
    const uint32_t nodeIndex,
    const std::vector<WorkingNode>& nodes,
    std::vector<uint32_t>& cache) {
    if (cache[nodeIndex] != 0U) {
        return cache[nodeIndex];
    }

    std::vector<std::pair<uint32_t, bool>> stack;
    stack.emplace_back(nodeIndex, false);

    while (!stack.empty()) {
        const auto [currentIndex, expanded] = stack.back();
        stack.pop_back();

        if (!expanded) {
            if (cache[currentIndex] != 0U) {
                continue;
            }

            stack.emplace_back(currentIndex, true);
            for (const uint32_t origin : nodes[currentIndex].incoming) {
                if (!nodes[origin].cycle && cache[origin] == 0U) {
                    stack.emplace_back(origin, false);
                }
            }
            continue;
        }

        uint32_t total = std::max<uint32_t>(1U, nodes[currentIndex].visits);
        for (const uint32_t origin : nodes[currentIndex].incoming) {
            if (!nodes[origin].cycle) {
                total += cache[origin];
            }
        }
        cache[currentIndex] = total;
    }

    return cache[nodeIndex];
}

std::vector<std::vector<uint32_t>> collectCycleComponents(const std::vector<WorkingNode>& nodes) {
    std::vector<std::vector<uint32_t>> components;
    std::vector<bool> visited(nodes.size(), false);

    for (uint32_t index = 0; index < nodes.size(); ++index) {
        if (!nodes[index].cycle || visited[index]) {
            continue;
        }

        std::vector<uint32_t> component;
        uint32_t current = index;
        do {
            component.push_back(current);
            visited[current] = true;
            current = nodes[current].next;
        } while (current != index && current != UINT32_MAX && !visited[current]);

        components.push_back(component);
    }

    return components;
}

float cycleLayoutRadius(const std::size_t cycleSize) {
    if (cycleSize <= 1U) {
        return 0.0f;
    }

    const float targetCircumference = static_cast<float>(cycleSize) * 18.0f;
    return std::max(kCycleRadius, targetCircumference / (2.0f * kPi));
}

uint32_t maxTreeDepth(
    const uint32_t nodeIndex,
    const std::vector<WorkingNode>& nodes,
    std::vector<uint32_t>& cache) {
    if (cache[nodeIndex] != UINT32_MAX) {
        return cache[nodeIndex];
    }

    std::vector<std::pair<uint32_t, bool>> stack;
    stack.emplace_back(nodeIndex, false);

    while (!stack.empty()) {
        const auto [currentIndex, expanded] = stack.back();
        stack.pop_back();

        if (!expanded) {
            if (cache[currentIndex] != UINT32_MAX) {
                continue;
            }

            stack.emplace_back(currentIndex, true);
            for (const uint32_t origin : nodes[currentIndex].incoming) {
                if (!nodes[origin].cycle && cache[origin] == UINT32_MAX) {
                    stack.emplace_back(origin, false);
                }
            }
            continue;
        }

        uint32_t bestDepth = 0U;
        for (const uint32_t origin : nodes[currentIndex].incoming) {
            if (!nodes[origin].cycle) {
                bestDepth = std::max(bestDepth, 1U + cache[origin]);
            }
        }
        cache[currentIndex] = bestDepth;
    }

    return cache[nodeIndex];
}

std::vector<uint32_t> collectSortedTreeChildren(
    const uint32_t parentIndex,
    const std::vector<WorkingNode>& nodes) {
    std::vector<uint32_t> children;
    for (const uint32_t origin : nodes[parentIndex].incoming) {
        if (!nodes[origin].cycle) {
            children.push_back(origin);
        }
    }

    if (children.empty()) {
        return children;
    }

    std::sort(
        children.begin(),
        children.end(),
        [&nodes](const uint32_t left, const uint32_t right) {
            if (nodes[left].visits != nodes[right].visits) {
                return nodes[left].visits > nodes[right].visits;
            }
            return left < right;
        });
    return children;
}

void layoutTree(
    const uint32_t parentIndex,
    const float baseRadius,
    const float angleStart,
    const float angleEnd,
    const uint32_t depth,
    std::vector<WorkingNode>& nodes,
    std::vector<uint32_t>& weights) {
    struct LayoutFrame {
        uint32_t parentIndex = 0U;
        float angleStart = 0.0f;
        float angleEnd = 0.0f;
        uint32_t depth = 0U;
    };

    std::vector<LayoutFrame> stack;
    stack.push_back(LayoutFrame{parentIndex, angleStart, angleEnd, depth});

    while (!stack.empty()) {
        const LayoutFrame frame = stack.back();
        stack.pop_back();

        const std::vector<uint32_t> children = collectSortedTreeChildren(frame.parentIndex, nodes);
        if (children.empty()) {
            continue;
        }

        uint32_t totalWeight = 0U;
        for (const uint32_t child : children) {
            totalWeight += std::max<uint32_t>(1U, weights[child]);
        }
        if (totalWeight == 0U) {
            totalWeight = static_cast<uint32_t>(children.size());
        }

        const float span = std::max(0.35f, frame.angleEnd - frame.angleStart);
        const float radius = std::max(
            kMinBranchRadius,
            baseRadius * std::pow(0.92f, static_cast<float>(frame.depth > 0 ? frame.depth - 1U : 0U)) +
                static_cast<float>(std::min<uint32_t>(frame.depth, 3U)) * 6.0f);
        float cursor = frame.angleStart;
        const float parentX = nodes[frame.parentIndex].x;
        const float parentY = nodes[frame.parentIndex].y;

        std::vector<LayoutFrame> childFrames;
        childFrames.reserve(children.size());
        for (const uint32_t child : children) {
            const float childSpan =
                span * (static_cast<float>(std::max<uint32_t>(1U, weights[child])) / static_cast<float>(totalWeight));
            const float childAngle = cursor + childSpan * 0.5f;
            nodes[child].x = parentX + radius * std::cos(childAngle);
            nodes[child].y = parentY + radius * std::sin(childAngle);

            const float nestedSpan = std::max(0.28f, childSpan * 0.92f);
            childFrames.push_back(LayoutFrame{
                child,
                childAngle - nestedSpan * 0.5f,
                childAngle + nestedSpan * 0.5f,
                frame.depth + 1U
            });
            cursor += childSpan;
        }

        for (auto it = childFrames.rbegin(); it != childFrames.rend(); ++it) {
            stack.push_back(*it);
        }
    }
}

void applyRadialLayout(std::vector<WorkingNode>& nodes) {
    if (nodes.empty()) {
        return;
    }

    buildIncomingLists(nodes);
    markCycles(nodes);
    const std::vector<std::vector<uint32_t>> components = collectCycleComponents(nodes);
    if (components.empty()) {
        return;
    }

    std::vector<uint32_t> weights(nodes.size(), 0U);
    for (uint32_t index = 0; index < nodes.size(); ++index) {
        if (!nodes[index].cycle) {
            subtreeWeight(index, nodes, weights);
        }
    }

    std::vector<uint32_t> depthCache(nodes.size(), UINT32_MAX);
    std::vector<float> componentExtents(components.size(), kMinComponentExtent);
    std::vector<float> cycleRadii(components.size(), 0.0f);
    std::vector<uint32_t> componentMasses(components.size(), 0U);

    for (std::size_t componentIndex = 0; componentIndex < components.size(); ++componentIndex) {
        const std::vector<uint32_t>& cycle = components[componentIndex];
        const float cycleRadius = cycleLayoutRadius(cycle.size());
        cycleRadii[componentIndex] = cycleRadius;

        uint32_t maxDepth = 0U;
        uint32_t componentMass = 0U;
        for (const uint32_t nodeIndex : cycle) {
            maxDepth = std::max(maxDepth, maxTreeDepth(nodeIndex, nodes, depthCache));
            componentMass += std::max<uint32_t>(1U, weights[nodeIndex]);
        }
        componentMasses[componentIndex] = componentMass;

        componentExtents[componentIndex] = std::max(
            kMinComponentExtent,
            cycleRadius + 28.0f + static_cast<float>(maxDepth) * kTreeLayerGap);
    }

    std::vector<std::size_t> orderedComponents(components.size(), 0U);
    std::iota(orderedComponents.begin(), orderedComponents.end(), 0U);
    std::stable_sort(
        orderedComponents.begin(),
        orderedComponents.end(),
        [&](const std::size_t left, const std::size_t right) {
            if (componentExtents[left] != componentExtents[right]) {
                return componentExtents[left] > componentExtents[right];
            }
            if (componentMasses[left] != componentMasses[right]) {
                return componentMasses[left] > componentMasses[right];
            }
            return left < right;
        });

    float maxComponentExtent = 0.0f;
    for (const float extent : componentExtents) {
        maxComponentExtent = std::max(maxComponentExtent, extent);
    }

    const float clusterRadius =
        orderedComponents.size() <= 1U ? 0.0f : std::max(36.0f, maxComponentExtent * kRootClusterOverlap);

    for (std::size_t orderedComponentIndex = 0; orderedComponentIndex < orderedComponents.size(); ++orderedComponentIndex) {
        const std::size_t componentIndex = orderedComponents[orderedComponentIndex];
        const float componentAngle =
            orderedComponents.size() == 1U
                ? 0.0f
                : static_cast<float>((2.0 * static_cast<double>(kPi) * static_cast<double>(orderedComponentIndex)) /
                                     static_cast<double>(orderedComponents.size()));
        const float componentCenterX = clusterRadius * std::cos(componentAngle);
        const float componentCenterY = clusterRadius * std::sin(componentAngle);

        const std::vector<uint32_t>& cycle = components[componentIndex];
        const float cycleRadius = cycleRadii[componentIndex];
        for (std::size_t index = 0; index < cycle.size(); ++index) {
            const float cycleAngle =
                (cycle.size() == 1U)
                    ? 0.0f
                    : static_cast<float>((2.0 * static_cast<double>(kPi) * static_cast<double>(index)) / static_cast<double>(cycle.size()));
            WorkingNode& node = nodes[cycle[index]];
            node.x = componentCenterX + (cycle.size() == 1U ? 0.0f : cycleRadius * std::cos(cycleAngle));
            node.y = componentCenterY + (cycle.size() == 1U ? 0.0f : cycleRadius * std::sin(cycleAngle));
        }

        for (std::size_t index = 0; index < cycle.size(); ++index) {
            const float sectorStart =
                static_cast<float>((2.0 * static_cast<double>(kPi) * static_cast<double>(index)) / static_cast<double>(cycle.size())) - kPi / static_cast<float>(cycle.size());
            const float sectorEnd =
                static_cast<float>((2.0 * static_cast<double>(kPi) * static_cast<double>(index + 1U)) / static_cast<double>(cycle.size())) - kPi / static_cast<float>(cycle.size());
            layoutTree(
                cycle[index],
                std::max(40.0f, cycleRadius + 20.0f),
                cycle.size() == 1U ? 0.0f : sectorStart,
                cycle.size() == 1U ? 2.0f * kPi : sectorEnd,
                1U,
                nodes,
                weights);
        }
    }
}

AttractorGraph makeGraphSnapshot(
    const AttractorSettings& settings,
    const std::vector<WorkingNode>& sourceNodes,
    const bool approximate,
    const uint64_t processedSeeds) {
    std::vector<WorkingNode> nodes = sourceNodes;
    applyRadialLayout(nodes);

    AttractorGraph graph{};
    graph.settings = settings;
    graph.approximate = approximate;
    graph.processedSeeds = processedSeeds;
    graph.nodes.reserve(nodes.size());
    graph.edges.reserve(nodes.size());

    for (const WorkingNode& node : nodes) {
        graph.nodes.push_back(AttractorGraphNode{
            node.key,
            node.x,
            node.y,
            node.next,
            node.visits,
            node.cycle
        });
    }
    for (uint32_t index = 0; index < nodes.size(); ++index) {
        if (nodes[index].next != UINT32_MAX) {
            graph.edges.emplace_back(index, nodes[index].next);
        }
    }

    return graph;
}

void rebuildWorkingGraphFromSnapshot(
    const AttractorGraph& graph,
    std::unordered_map<AttractorStateBlock, uint32_t, AttractorStateBlockHash>& keyToNode,
    std::vector<WorkingNode>& nodes) {
    nodes.clear();
    keyToNode.clear();
    nodes.reserve(graph.nodes.size());
    keyToNode.reserve(graph.nodes.size() * 2U);

    for (std::size_t index = 0; index < graph.nodes.size(); ++index) {
        const AttractorGraphNode& source = graph.nodes[index];
        WorkingNode node{};
        node.key = source.key;
        node.next = source.next;
        node.visits = source.visits;
        node.cycle = source.cycle;
        node.x = source.x;
        node.y = source.y;
        nodes.push_back(std::move(node));
        keyToNode.emplace(source.key, static_cast<uint32_t>(index));
    }
    buildIncomingLists(nodes);
}

uint64_t approximateSeedBase(const AttractorSettings& settings) {
    uint64_t seed = 0x6a09e667f3bcc909ULL;
    seed ^= static_cast<uint64_t>(settings.width) * 0x9e3779b97f4a7c15ULL;
    seed ^= static_cast<uint64_t>(settings.height) * 0xc2b2ae3d27d4eb4fULL;
    seed ^= settings.approximateSampleBudget * 0x165667b19e3779f9ULL;
    return mix64(seed);
}

AttractorGraph buildAttractorGraph(
    const AttractorSettings& settings,
    const bool approximate,
    const uint64_t startSeedIndex,
    const uint64_t targetSeedIndex,
    const uint64_t seedBase,
    const std::optional<AttractorGraph>& initialGraph,
    const ProgressCallback& reportProgress,
    const GraphCallback& publishGraph,
    const BatchSuccessorEvaluator& successorEvaluator,
    const std::function<bool()>& isCancelled) {
    const std::size_t totalCells = cellCount(settings);
    if (totalCells == 0U) {
        throw std::runtime_error("Attractor subgrid must be at least 1x1.");
    }

    std::unordered_map<AttractorStateBlock, uint32_t, AttractorStateBlockHash> keyToNode;
    std::unordered_map<AttractorStateBlock, AttractorStateBlock, AttractorStateBlockHash> successorCache;
    std::vector<WorkingNode> nodes;
    if (initialGraph.has_value()) {
        rebuildWorkingGraphFromSnapshot(initialGraph.value(), keyToNode, nodes);
    } else {
        keyToNode.reserve(static_cast<std::size_t>(std::min<uint64_t>(targetSeedIndex * 4ULL, 2'000'000ULL)));
        nodes.reserve(static_cast<std::size_t>(std::min<uint64_t>(targetSeedIndex * 2ULL, 2'000'000ULL)));
    }
    successorCache.reserve(static_cast<std::size_t>(std::min<uint64_t>(targetSeedIndex * 6ULL, 4'000'000ULL)));

    AttractorProgress progress{};
    progress.running = true;
    progress.approximate = approximate;
    progress.canRefine = false;
    progress.processedSeeds = startSeedIndex;
    progress.totalSeeds = targetSeedIndex;
    progress.discoveredNodes = nodes.size();
    progress.message = approximate ? "ATR APROX" : "GENERANDO";
    reportProgress(progress);

    auto lastPreviewTime = std::chrono::steady_clock::now();

    const auto makeSeed = [&](const uint64_t seedIndex) {
        return approximate
            ? sampleApproximateSeed(settings, seedBase, seedIndex)
            : encodeSeedState(seedIndex, settings);
    };

    for (uint64_t batchStart = startSeedIndex; batchStart < targetSeedIndex; batchStart += static_cast<uint64_t>(kSeedBatchSize)) {
        if (isCancelled()) {
            throw std::runtime_error("Attractor generation cancelled.");
        }

        const uint64_t batchEnd = std::min<uint64_t>(targetSeedIndex, batchStart + static_cast<uint64_t>(kSeedBatchSize));
        std::vector<SeedWalker> walkers(static_cast<std::size_t>(batchEnd - batchStart));
        for (uint64_t seed = batchStart; seed < batchEnd; ++seed) {
            SeedWalker& walker = walkers[static_cast<std::size_t>(seed - batchStart)];
            walker.currentKey = makeSeed(seed);
            walker.localPath.reserve(128);
            walker.localSeen.reserve(128);
        }

        std::size_t finishedWalkers = 0U;
        std::size_t lastReportedFinishedWalkers = 0U;

        while (finishedWalkers < walkers.size()) {
            if (isCancelled()) {
                throw std::runtime_error("Attractor generation cancelled.");
            }

            std::vector<AttractorStateBlock> unresolvedKeys;
            std::unordered_set<AttractorStateBlock, AttractorStateBlockHash> unresolvedUnique;
            unresolvedUnique.reserve(walkers.size() * 2U);

            for (SeedWalker& walker : walkers) {
                if (walker.finished) {
                    continue;
                }

                while (true) {
                    const auto globalFound = keyToNode.find(walker.currentKey);
                    if (globalFound != keyToNode.end()) {
                        connectLocalPathToExisting(walker.localPath, globalFound->second, keyToNode, nodes);
                        walker.finished = true;
                        ++finishedWalkers;
                        break;
                    }

                    const auto localFound = walker.localSeen.find(walker.currentKey);
                    if (localFound != walker.localSeen.end()) {
                        connectLocalCycle(walker.localPath, localFound->second, keyToNode, nodes);
                        walker.finished = true;
                        ++finishedWalkers;
                        break;
                    }

                    const auto successorFound = successorCache.find(walker.currentKey);
                    if (successorFound == successorCache.end()) {
                        if (unresolvedUnique.emplace(walker.currentKey).second) {
                            unresolvedKeys.push_back(walker.currentKey);
                        }
                        break;
                    }

                    walker.localSeen.emplace(walker.currentKey, walker.localPath.size());
                    walker.localPath.push_back(walker.currentKey);
                    walker.currentKey = successorFound->second;
                }
            }

            if (!unresolvedKeys.empty()) {
                std::vector<AttractorStateBlock> successors;
                if (successorEvaluator) {
                    successorEvaluator(settings, unresolvedKeys, successors);
                } else {
                    evaluateSuccessorBatchCpu(settings, unresolvedKeys, successors);
                }
                if (successors.size() != unresolvedKeys.size()) {
                    throw std::runtime_error("Attractor successor batch size mismatch.");
                }

                for (std::size_t index = 0; index < unresolvedKeys.size(); ++index) {
                    successorCache.emplace(unresolvedKeys[index], successors[index]);
                }
            }

            const auto now = std::chrono::steady_clock::now();
            const bool shouldReport =
                finishedWalkers == walkers.size() ||
                finishedWalkers >= lastReportedFinishedWalkers + 32U ||
                now - lastPreviewTime >= std::chrono::milliseconds(120);
            if (shouldReport) {
                progress.processedSeeds = batchStart + finishedWalkers;
                progress.discoveredNodes = nodes.size();
                reportProgress(progress);
                lastReportedFinishedWalkers = finishedWalkers;
            }

            const bool shouldPublishPreview =
                !nodes.empty() &&
                (finishedWalkers == walkers.size() || now - lastPreviewTime >= std::chrono::milliseconds(120));
            if (shouldPublishPreview) {
                publishGraph(makeGraphSnapshot(settings, nodes, approximate, batchStart + finishedWalkers));
                lastPreviewTime = now;
            }
        }
    }

    return makeGraphSnapshot(settings, nodes, approximate, targetSeedIndex);
}

}  // namespace

AttractorGenerator::~AttractorGenerator() {
    cancel();
    joinWorker();
}

bool AttractorGenerator::start(
    const AttractorSettings& settings,
    BatchSuccessorEvaluator successorEvaluator,
    const bool refine) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (progress_.running) {
            return false;
        }
        if (refine) {
            if (!approximateContinuation_.has_value()) {
                return false;
            }
            if (approximateContinuation_->graph.settings.width != settings.width ||
                approximateContinuation_->graph.settings.height != settings.height) {
                return false;
            }
        }
    }

    joinWorker();

    const bool approximate = usesApproximateSampling(settings);

    std::lock_guard<std::mutex> lock(mutex_);
    cancelRequested_ = false;
    if (!refine) {
        availableGraph_.reset();
        if (!approximate) {
            approximateContinuation_.reset();
        }
    }
    progress_ = {};
    progress_.running = true;
    progress_.approximate = approximate;
    progress_.message = approximate ? "ATR APROX" : "GENERANDO";

    worker_ = std::thread(&AttractorGenerator::runWorker, this, settings, std::move(successorEvaluator), refine);
    return true;
}

void AttractorGenerator::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    cancelRequested_ = true;
}

void AttractorGenerator::shutdown() {
    cancel();
    joinWorker();
}

AttractorProgress AttractorGenerator::progress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return progress_;
}

std::optional<AttractorGraph> AttractorGenerator::takeLatestGraph() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!availableGraph_.has_value()) {
        return std::nullopt;
    }

    std::optional<AttractorGraph> result = std::move(availableGraph_);
    availableGraph_.reset();
    return result;
}

void AttractorGenerator::joinWorker() {
    if (worker_.joinable()) {
        worker_.join();
    }
}

void AttractorGenerator::runWorker(
    const AttractorSettings settings,
    BatchSuccessorEvaluator successorEvaluator,
    const bool refine) {
    try {
        const bool approximate = usesApproximateSampling(settings);
        std::optional<ApproximateContinuation> continuation;
        if (approximate) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (refine) {
                continuation = approximateContinuation_;
            }
        }

        const uint64_t totalCells = static_cast<uint64_t>(cellCount(settings));
        const uint64_t totalSeeds =
            approximate ? 0ULL : powChecked(static_cast<uint64_t>(kVisibleStatesPerCell), static_cast<uint32_t>(totalCells));
        const uint64_t startSeedIndex = continuation.has_value() ? continuation->processedSeeds : 0ULL;
        const uint64_t targetSeedIndex =
            approximate
                ? startSeedIndex + std::max<uint64_t>(1ULL, settings.approximateSampleBudget)
                : totalSeeds;
        const uint64_t seedBase =
            continuation.has_value() ? continuation->seedBase : approximateSeedBase(settings);

        const auto reportProgress = [this](const AttractorProgress& progress) {
            std::lock_guard<std::mutex> lock(mutex_);
            progress_ = progress;
        };
        const auto publishGraph = [this](AttractorGraph graph) {
            std::lock_guard<std::mutex> lock(mutex_);
            availableGraph_ = std::move(graph);
        };
        const auto isCancelled = [this]() {
            std::lock_guard<std::mutex> lock(mutex_);
            return cancelRequested_;
        };

        AttractorGraph graph = buildAttractorGraph(
            settings,
            approximate,
            startSeedIndex,
            targetSeedIndex,
            seedBase,
            continuation.has_value() ? std::optional<AttractorGraph>(continuation->graph) : std::nullopt,
            reportProgress,
            publishGraph,
            successorEvaluator,
            isCancelled);
        const uint64_t finalNodeCount = graph.nodes.size();

        std::lock_guard<std::mutex> lock(mutex_);
        availableGraph_ = graph;
        if (approximate) {
            approximateContinuation_ = ApproximateContinuation{
                graph,
                graph.processedSeeds,
                seedBase
            };
        } else {
            approximateContinuation_.reset();
        }
        progress_.running = false;
        progress_.completed = true;
        progress_.hasError = false;
        progress_.approximate = approximate;
        progress_.canRefine = approximate;
        progress_.message = approximate ? "ATR APROX" : "ATR LISTO";
        progress_.discoveredNodes = finalNodeCount;
        progress_.processedSeeds = graph.processedSeeds;
        progress_.totalSeeds = graph.processedSeeds;
    } catch (const std::exception& exception) {
        std::lock_guard<std::mutex> lock(mutex_);
        availableGraph_.reset();
        progress_.running = false;
        progress_.completed = true;
        progress_.hasError = true;
        progress_.canRefine = approximateContinuation_.has_value();
        progress_.message = exception.what();
    }
}
