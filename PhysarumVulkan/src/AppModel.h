#pragma once

#include "PhysarumSim.h"

#include <chrono>
#include <cstdint>

class AppModel {
public:
    explicit AppModel(GridSize initialGridSize);

    [[nodiscard]] PhysarumSim& simulation() {
        return simulation_;
    }

    [[nodiscard]] const PhysarumSim& simulation() const {
        return simulation_;
    }

    [[nodiscard]] GridSize& requestedGridSize() {
        return requestedGridSize_;
    }

    [[nodiscard]] const GridSize& requestedGridSize() const {
        return requestedGridSize_;
    }

    [[nodiscard]] bool& play() {
        return play_;
    }

    [[nodiscard]] const bool& play() const {
        return play_;
    }

    [[nodiscard]] uint64_t& generation() {
        return generation_;
    }

    [[nodiscard]] const uint64_t& generation() const {
        return generation_;
    }

    [[nodiscard]] std::chrono::steady_clock::time_point& lastSimulationStep() {
        return lastSimulationStep_;
    }

    [[nodiscard]] const std::chrono::steady_clock::time_point& lastSimulationStep() const {
        return lastSimulationStep_;
    }

    void togglePlayback();
    void pause();
    void stopAndReset(std::chrono::steady_clock::time_point now);
    [[nodiscard]] bool shouldAdvanceSimulation(
        std::chrono::steady_clock::time_point now,
        std::chrono::milliseconds stepInterval) const;
    void commitSimulationAdvance(std::chrono::steady_clock::time_point now, bool routed);
    bool advanceSimulation(
        std::chrono::steady_clock::time_point now,
        std::chrono::milliseconds stepInterval);
    void resizeGrid(GridSize newSize);

private:
    PhysarumSim simulation_;
    GridSize requestedGridSize_{};
    bool play_ = false;
    uint64_t generation_ = 0;
    std::chrono::steady_clock::time_point lastSimulationStep_{};
};
