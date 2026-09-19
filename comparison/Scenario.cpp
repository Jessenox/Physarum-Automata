#include "Scenario.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace comparison {

uint32_t Scenario::nodeCount() const {
    const uint64_t count = static_cast<uint64_t>(width) * height;
    if (count >= std::numeric_limits<uint32_t>::max()) {
        throw std::invalid_argument("Comparison scenario exceeds 32-bit node indices.");
    }
    return static_cast<uint32_t>(count);
}

bool Scenario::traversable(const uint32_t index) const {
    return index < nodeCount() && !std::binary_search(obstacles.begin(), obstacles.end(), index);
}

bool Scenario::isGoal(const uint32_t index) const {
    return std::binary_search(goals.begin(), goals.end(), index);
}

void Scenario::validate() const {
    if (width < 8U || height < 8U) throw std::invalid_argument("Comparison grid must be at least 8x8.");
    if (source >= nodeCount() || goals.empty()) {
        throw std::invalid_argument("Scenario has invalid endpoints.");
    }
    if (!std::is_sorted(goals.begin(), goals.end())
        || std::adjacent_find(goals.begin(), goals.end()) != goals.end()
        || !std::is_sorted(obstacles.begin(), obstacles.end())
        || std::adjacent_find(obstacles.begin(), obstacles.end()) != obstacles.end()) {
        throw std::invalid_argument("Scenario goals and obstacles must be sorted and unique.");
    }
    if (!traversable(source)) {
        throw std::invalid_argument("Scenario endpoints must be traversable.");
    }
    for (const uint32_t goal : goals) {
        if (goal >= nodeCount() || goal == source || !traversable(goal)) {
            throw std::invalid_argument("Scenario has an invalid goal.");
        }
    }
    for (const uint32_t obstacle : obstacles) {
        if (obstacle >= nodeCount()) throw std::invalid_argument("Scenario has an obstacle outside the grid.");
    }
}

Scenario generateScenario(const uint32_t width, const uint32_t height, const uint32_t seed) {
    Scenario scenario;
    scenario.width = width;
    scenario.height = height;
    scenario.seed = seed;
    scenario.source = (height / 2U) * width + 1U;
    scenario.goals.push_back((height / 2U) * width + width - 2U);
    scenario.validate();
    return scenario;
}

Scenario loadScenario(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Could not open comparison scenario: " + path.string());
    std::string magic;
    uint32_t version = 0U;
    input >> magic >> version;
    if (magic != "PACGRID" || (version != 1U && version != 2U && version != 3U)) {
        throw std::runtime_error("Unsupported PACGRID file.");
    }
    Scenario scenario;
    std::string label;
    uint32_t sourceX = 0U;
    uint32_t sourceY = 0U;
    input >> label >> scenario.width >> scenario.height;
    if (label != "size") throw std::runtime_error("PACGRID expected size field.");
    if (scenario.width < 8U || scenario.height < 8U
        || static_cast<uint64_t>(scenario.width) * scenario.height >= std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("PACGRID dimensions are outside the 32-bit grid range.");
    }
    const auto coordinateNode = [&](const uint32_t x, const uint32_t y, const char* field) {
        if (x >= scenario.width || y >= scenario.height) {
            throw std::runtime_error(std::string("PACGRID ") + field + " coordinate lies outside the grid.");
        }
        return y * scenario.width + x;
    };
    input >> label >> sourceX >> sourceY;
    if (label != "start") throw std::runtime_error("PACGRID expected start field.");
    scenario.source = coordinateNode(sourceX, sourceY, "start");
    if (version == 1U) {
        uint32_t targetX = 0U;
        uint32_t targetY = 0U;
        input >> label >> targetX >> targetY;
        if (label != "goal") throw std::runtime_error("PACGRID expected goal field.");
        scenario.goals.push_back(coordinateNode(targetX, targetY, "goal"));
        input >> label >> scenario.seed;
        if (label != "seed") throw std::runtime_error("PACGRID expected seed field.");
        input >> label;
        if (label != "cells") throw std::runtime_error("PACGRID expected cells section.");
        for (uint32_t y = 0U; y < scenario.height; ++y) {
            std::string row;
            input >> row;
            if (row.size() != scenario.width) throw std::runtime_error("PACGRID row width mismatch.");
            for (uint32_t x = 0U; x < scenario.width; ++x) {
                const char cell = row[x];
                if (cell != '#' && cell != '.' && cell != 'S' && cell != 'G') {
                    throw std::runtime_error("PACGRID contains an unknown cell symbol.");
                }
                if (cell == '#') scenario.obstacles.push_back(y * scenario.width + x);
            }
        }
    } else {
        uint32_t count = 0U;
        input >> label >> count;
        if (label != "goals") throw std::runtime_error("PACGRID expected goals field.");
        scenario.goals.reserve(count);
        for (uint32_t index = 0U; index < count; ++index) {
            uint32_t x = 0U;
            uint32_t y = 0U;
            input >> x >> y;
            scenario.goals.push_back(coordinateNode(x, y, "goal"));
        }
        input >> label >> scenario.seed;
        if (label != "seed") throw std::runtime_error("PACGRID expected seed field.");
        input >> label >> count;
        if (label != "obstacles") throw std::runtime_error("PACGRID expected obstacles field.");
        scenario.obstacles.reserve(count);
        if (version == 2U) {
            for (uint32_t index = 0U; index < count; ++index) {
                uint32_t x = 0U;
                uint32_t y = 0U;
                input >> x >> y;
                scenario.obstacles.push_back(coordinateNode(x, y, "obstacle"));
            }
        } else {
            uint32_t runCount = 0U;
            input >> label >> runCount;
            if (label != "runs") throw std::runtime_error("PACGRID 3 expected runs field.");
            for (uint32_t index = 0U; index < runCount; ++index) {
                uint32_t y = 0U;
                uint32_t firstX = 0U;
                uint32_t length = 0U;
                input >> y >> firstX >> length;
                if (length == 0U || y >= scenario.height || firstX >= scenario.width
                    || static_cast<uint64_t>(firstX) + length > scenario.width) {
                    throw std::runtime_error("PACGRID 3 contains an invalid obstacle run.");
                }
                for (uint32_t offset = 0U; offset < length; ++offset) {
                    scenario.obstacles.push_back(y * scenario.width + firstX + offset);
                }
            }
            if (scenario.obstacles.size() != count) {
                throw std::runtime_error("PACGRID 3 obstacle count does not match its runs.");
            }
        }
    }
    if (!input) throw std::runtime_error("PACGRID is incomplete.");
    if (!std::is_sorted(scenario.goals.begin(), scenario.goals.end())) {
        std::sort(scenario.goals.begin(), scenario.goals.end());
    }
    scenario.goals.erase(std::unique(scenario.goals.begin(), scenario.goals.end()), scenario.goals.end());
    if (!std::is_sorted(scenario.obstacles.begin(), scenario.obstacles.end())) {
        std::sort(scenario.obstacles.begin(), scenario.obstacles.end());
    }
    scenario.obstacles.erase(std::unique(scenario.obstacles.begin(), scenario.obstacles.end()),
                             scenario.obstacles.end());
    scenario.validate();
    return scenario;
}

void saveScenario(const Scenario& scenario, const std::filesystem::path& path) {
    scenario.validate();
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    if (!output) throw std::runtime_error("Could not write comparison scenario: " + path.string());
    output << "PACGRID 2\n"
           << "size " << scenario.width << ' ' << scenario.height << '\n'
           << "start " << scenario.source % scenario.width << ' ' << scenario.source / scenario.width << '\n'
           << "goals " << scenario.goals.size() << '\n';
    for (const uint32_t goal : scenario.goals) {
        output << goal % scenario.width << ' ' << goal / scenario.width << '\n';
    }
    output << "seed " << scenario.seed << '\n'
           << "obstacles " << scenario.obstacles.size() << '\n';
    for (const uint32_t obstacle : scenario.obstacles) {
        output << obstacle % scenario.width << ' ' << obstacle / scenario.width << '\n';
    }
}

}  // namespace comparison
