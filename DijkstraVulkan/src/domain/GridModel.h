#pragma once

#include <cstdint>
#include <optional>
#include <vector>

struct GridSize {
    uint32_t width = 64;
    uint32_t height = 64;
};

struct DijkstraResult {
    bool found = false;
    uint32_t distance = 0;
    uint32_t visitedNodes = 0;
    std::vector<uint32_t> path;
};

class GridModel {
public:
    explicit GridModel(GridSize size);

    [[nodiscard]] GridSize size() const;
    [[nodiscard]] uint32_t nodeCount() const;
    [[nodiscard]] uint32_t source() const;
    [[nodiscard]] uint32_t target() const;
    [[nodiscard]] const std::vector<uint8_t>& weights() const;
    [[nodiscard]] uint8_t weight(uint32_t index) const;

    void setSource(uint32_t index);
    void setTarget(uint32_t index);
    void setWeight(uint32_t index, uint32_t weight);
    void clearWalls();
    void randomizeWalls(uint32_t seed, float density = 0.24f);

    [[nodiscard]] DijkstraResult solveCpu() const;

private:
    [[nodiscard]] bool validIndex(uint32_t index) const;

    GridSize size_{};
    std::vector<uint8_t> weights_;
    uint32_t source_ = 0;
    uint32_t target_ = 0;
};
