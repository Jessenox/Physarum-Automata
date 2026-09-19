#pragma once

#include <cstdint>

struct SolidDrawRange {
    uint32_t firstVertex = 0;
    uint32_t vertexCount = 0;
};

struct GraphDrawRanges {
    SolidDrawRange edges{};
    SolidDrawRange nodes{};
    SolidDrawRange cycles{};
    SolidDrawRange legendPanel{};
    SolidDrawRange legendEdgeSwatch{};
    SolidDrawRange legendNodeSwatch{};
    SolidDrawRange legendCycleSwatch{};
    SolidDrawRange legendText{};
};
