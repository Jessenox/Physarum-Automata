#include "domain/GridModel.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void require(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void openGridUsesManhattanDistance() {
    GridModel grid({5, 5});
    grid.setSource(0);
    grid.setTarget(24);
    const DijkstraResult result = grid.solveCpu();
    require(result.found, "open grid should have a path");
    require(result.distance == 8U, "open 5x5 corner path should cost 8");
    require(result.path.front() == 0U && result.path.back() == 24U, "path endpoints should match");
}

void wallsCanMakeTargetUnreachable() {
    GridModel grid({3, 3});
    grid.setSource(0);
    grid.setTarget(8);
    grid.setWeight(1, 0);
    grid.setWeight(3, 0);
    const DijkstraResult result = grid.solveCpu();
    require(!result.found, "blocked source should have no path");
}

void weightsChangeTheChosenRoute() {
    GridModel grid({4, 2});
    grid.setSource(0);
    grid.setTarget(3);
    grid.setWeight(1, 5);
    grid.setWeight(2, 5);
    const DijkstraResult result = grid.solveCpu();
    require(result.found, "weighted grid should have a path");
    require(result.distance == 5U, "detour should be cheaper than two weight-5 cells");
    require(result.path.size() == 6U, "weighted route should use the lower row detour");
}

void gridsAboveTheFormerLimitAreAccepted() {
    GridModel grid({1024, 1024});
    require(grid.nodeCount() == 1'048'576U, "grid above the former one-million limit should be accepted");
}

void overflowingIndexSpaceIsRejectedBeforeAllocation() {
    bool rejected = false;
    try {
        GridModel grid({100'000, 100'000});
        (void)grid;
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "grid larger than uint32 index space should be rejected");
}

}  // namespace

int main() {
    openGridUsesManhattanDistance();
    wallsCanMakeTargetUnreachable();
    weightsChangeTheChosenRoute();
    gridsAboveTheFormerLimitAreAccepted();
    overflowingIndexSpaceIsRejectedBeforeAllocation();
    std::cout << "GridModel tests passed\n";
    return 0;
}
