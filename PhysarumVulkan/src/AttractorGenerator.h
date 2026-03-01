#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct AttractorStateBlock {
    static constexpr std::size_t kMaxCells = 25U;
    std::array<uint8_t, kMaxCells> cells{};

    [[nodiscard]] bool operator==(const AttractorStateBlock&) const = default;
};

struct AttractorSettings {
    uint32_t width = 2;
    uint32_t height = 2;
    uint64_t approximateSampleBudget = 20'000ULL;
};

struct AttractorProgress {
    bool running = false;
    bool completed = false;
    bool hasError = false;
    bool approximate = false;
    bool canRefine = false;
    uint64_t processedSeeds = 0;
    uint64_t totalSeeds = 0;
    uint64_t discoveredNodes = 0;
    std::string message = "LISTO";
};

struct AttractorGraphNode {
    AttractorStateBlock key{};
    float x = 0.0f;
    float y = 0.0f;
    uint32_t next = UINT32_MAX;
    uint32_t visits = 0;
    bool cycle = false;
};

struct AttractorGraph {
    AttractorSettings settings{};
    bool approximate = false;
    uint64_t processedSeeds = 0;
    std::vector<AttractorGraphNode> nodes;
    std::vector<std::pair<uint32_t, uint32_t>> edges;
};

using BatchSuccessorEvaluator =
    std::function<void(const AttractorSettings&, const std::vector<AttractorStateBlock>&, std::vector<AttractorStateBlock>&)>;

class AttractorGenerator {
public:
    AttractorGenerator() = default;
    ~AttractorGenerator();

    AttractorGenerator(const AttractorGenerator&) = delete;
    AttractorGenerator& operator=(const AttractorGenerator&) = delete;

    bool start(const AttractorSettings& settings, BatchSuccessorEvaluator successorEvaluator = {}, bool refine = false);
    void cancel();
    void shutdown();
    [[nodiscard]] AttractorProgress progress() const;
    std::optional<AttractorGraph> takeLatestGraph();

private:
    void joinWorker();
    void runWorker(AttractorSettings settings, BatchSuccessorEvaluator successorEvaluator, bool refine);

    struct ApproximateContinuation {
        AttractorGraph graph{};
        uint64_t processedSeeds = 0;
        uint64_t seedBase = 0;
    };

private:
    mutable std::mutex mutex_{};
    std::thread worker_{};
    AttractorProgress progress_{};
    std::optional<AttractorGraph> availableGraph_{};
    std::optional<ApproximateContinuation> approximateContinuation_{};
    bool cancelRequested_ = false;
};
