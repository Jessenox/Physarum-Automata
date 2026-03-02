#include "AppModel.h"

AppModel::AppModel(const GridSize initialGridSize)
    : simulation_(initialGridSize),
      requestedGridSize_(initialGridSize) {
}

void AppModel::togglePlayback() {
    play_ = !play_;
}

void AppModel::pause() {
    play_ = false;
}

void AppModel::stopAndReset(const std::chrono::steady_clock::time_point now) {
    play_ = false;
    generation_ = 0;
    lastSimulationStep_ = now;
}

bool AppModel::shouldAdvanceSimulation(
    const std::chrono::steady_clock::time_point now,
    const std::chrono::milliseconds stepInterval) const {
    return play_ && (now - lastSimulationStep_ >= stepInterval);
}

void AppModel::commitSimulationAdvance(
    const std::chrono::steady_clock::time_point now,
    const bool routed) {
    lastSimulationStep_ = now;
    ++generation_;
    if (routed) {
        play_ = false;
    }
}

bool AppModel::advanceSimulation(
    const std::chrono::steady_clock::time_point now,
    const std::chrono::milliseconds stepInterval) {
    if (!shouldAdvanceSimulation(now, stepInterval)) {
        return false;
    }
    simulation_.evaluatePhysarum();
    commitSimulationAdvance(now, simulation_.routed());

    return true;
}

void AppModel::resizeGrid(const GridSize newSize) {
    simulation_.resizeGrid(newSize);
    requestedGridSize_ = newSize;
}
