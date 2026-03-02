#pragma once

#include "AttractorGenerator.h"

#include <filesystem>

class AttractorGraphExporter {
public:
    [[nodiscard]] static bool pngSupported();

    void writeSvg(const std::filesystem::path& outputPath, const AttractorGraph& graph) const;
    void writePng(const std::filesystem::path& outputPath, const AttractorGraph& graph) const;
};
