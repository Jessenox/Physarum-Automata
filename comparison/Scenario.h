#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace comparison {

struct Scenario {
    uint32_t width = 32U;
    uint32_t height = 32U;
    uint32_t source = 0U;
    uint32_t seed = 12345U;
    std::vector<uint32_t> goals;
    std::vector<uint32_t> obstacles;

    [[nodiscard]] uint32_t nodeCount() const;
    [[nodiscard]] bool traversable(uint32_t index) const;
    [[nodiscard]] bool isGoal(uint32_t index) const;
    void validate() const;
};

[[nodiscard]] Scenario generateScenario(uint32_t width, uint32_t height, uint32_t seed);
[[nodiscard]] Scenario loadScenario(const std::filesystem::path& path);
void saveScenario(const Scenario& scenario, const std::filesystem::path& path);

}  // namespace comparison
