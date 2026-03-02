#include "infrastructure/AttractorGraphExporter.h"

#ifndef PHYSARUM_VULKAN_HAS_OPENCV
#define PHYSARUM_VULKAN_HAS_OPENCV 0
#endif

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

#if PHYSARUM_VULKAN_HAS_OPENCV
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#endif

bool AttractorGraphExporter::pngSupported() {
    return PHYSARUM_VULKAN_HAS_OPENCV != 0;
}

void AttractorGraphExporter::writeSvg(const std::filesystem::path& outputPath, const AttractorGraph& graph) const {
    if (graph.nodes.empty()) {
        throw std::runtime_error("No attractor graph available for export.");
    }

    constexpr float kPadding = 34.0f;
    float minX = graph.nodes.front().x;
    float minY = graph.nodes.front().y;
    float maxX = graph.nodes.front().x;
    float maxY = graph.nodes.front().y;
    for (const AttractorGraphNode& node : graph.nodes) {
        minX = std::min(minX, node.x);
        minY = std::min(minY, node.y);
        maxX = std::max(maxX, node.x);
        maxY = std::max(maxY, node.y);
    }

    minX -= kPadding;
    minY -= kPadding;
    maxX += kPadding;
    maxY += kPadding;
    const float width = std::max(120.0f, maxX - minX);
    const float height = std::max(120.0f, maxY - minY);

    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    std::ofstream output(outputPath);
    if (!output.is_open()) {
        throw std::runtime_error("Unable to open SVG output file.");
    }

    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
           << "width=\"" << static_cast<int>(std::lround(width * 2.0f)) << "\" "
           << "height=\"" << static_cast<int>(std::lround(height * 2.0f + 96.0f)) << "\" "
           << "viewBox=\"" << minX << ' ' << (minY - 68.0f) << ' ' << width << ' ' << (height + 96.0f) << "\">\n";
    output << "  <rect x=\"" << minX << "\" y=\"" << (minY - 68.0f) << "\" width=\"" << width
           << "\" height=\"" << (height + 96.0f) << "\" fill=\"#f1f4f7\"/>\n";
    output << "  <rect x=\"" << minX + 8.0f << "\" y=\"" << (minY - 58.0f) << "\" width=\"" << (width - 16.0f)
           << "\" height=\"" << (height + 76.0f)
           << "\" rx=\"8\" fill=\"#ffffff\" stroke=\"#e2e7ec\" stroke-width=\"1.5\"/>\n";
    output << "  <text x=\"" << (minX + 22.0f) << "\" y=\"" << (minY - 24.0f)
           << "\" font-family=\"monospace\" font-size=\"18\" font-weight=\"700\" fill=\"#1d2329\">ATRACTORES</text>\n";
    output << "  <text x=\"" << (minX + 22.0f) << "\" y=\"" << (minY + 2.0f)
           << "\" font-family=\"monospace\" font-size=\"10\" fill=\"#586068\">";
    output << graph.settings.width << 'x' << graph.settings.height
           << " | " << (graph.approximate ? "MODO APROX" : "MODO EXACTO")
           << " | PROC " << graph.processedSeeds
           << " | NOD " << graph.nodes.size()
           << " | EDGE " << graph.edges.size()
           << "</text>\n";

    for (const auto& [origin, destination] : graph.edges) {
        if (origin >= graph.nodes.size() || destination >= graph.nodes.size()) {
            continue;
        }

        const AttractorGraphNode& from = graph.nodes[origin];
        const AttractorGraphNode& to = graph.nodes[destination];
        output << "  <line x1=\"" << from.x << "\" y1=\"" << from.y
               << "\" x2=\"" << to.x << "\" y2=\"" << to.y
               << "\" stroke=\"#b08444\" stroke-width=\"1.8\" stroke-linecap=\"round\" opacity=\"0.92\"/>\n";
    }

    for (const AttractorGraphNode& node : graph.nodes) {
        const float radius = node.cycle ? 4.8f : 3.2f;
        const char* fill = node.cycle ? "#2eaaff" : "#2870cd";
        output << "  <circle cx=\"" << node.x << "\" cy=\"" << node.y
               << "\" r=\"" << (radius + 1.6f) << "\" fill=\"#ffffff\" opacity=\"0.96\"/>\n";
        output << "  <circle cx=\"" << node.x << "\" cy=\"" << node.y
               << "\" r=\"" << radius << "\" fill=\"" << fill
               << "\" stroke=\"#172131\" stroke-width=\"0.8\"/>\n";
    }

    output << "</svg>\n";
}

void AttractorGraphExporter::writePng(const std::filesystem::path& outputPath, const AttractorGraph& graph) const {
#if !PHYSARUM_VULKAN_HAS_OPENCV
    (void)outputPath;
    (void)graph;
    throw std::runtime_error("PNG export requires OpenCV support.");
#else
    if (graph.nodes.empty()) {
        throw std::runtime_error("No attractor graph available for PNG export.");
    }

    constexpr int kImageWidth = 1600;
    constexpr int kImageHeight = 1080;
    constexpr float kPadding = 32.0f;
    constexpr float kMarginLeft = 72.0f;
    constexpr float kMarginTop = 88.0f;
    constexpr float kMarginRight = 72.0f;
    constexpr float kMarginBottom = 72.0f;

    float minX = graph.nodes.front().x;
    float minY = graph.nodes.front().y;
    float maxX = graph.nodes.front().x;
    float maxY = graph.nodes.front().y;
    for (const AttractorGraphNode& node : graph.nodes) {
        minX = std::min(minX, node.x);
        minY = std::min(minY, node.y);
        maxX = std::max(maxX, node.x);
        maxY = std::max(maxY, node.y);
    }

    minX -= kPadding;
    minY -= kPadding;
    maxX += kPadding;
    maxY += kPadding;

    const float worldWidth = std::max(180.0f, maxX - minX);
    const float worldHeight = std::max(180.0f, maxY - minY);
    const float contentWidth = static_cast<float>(kImageWidth) - kMarginLeft - kMarginRight;
    const float contentHeight = static_cast<float>(kImageHeight) - kMarginTop - kMarginBottom;
    const float scale = std::min(contentWidth / worldWidth, contentHeight / worldHeight);
    const float offsetX = kMarginLeft + (contentWidth - worldWidth * scale) * 0.5f;
    const float offsetY = kMarginTop + (contentHeight - worldHeight * scale) * 0.5f;

    const auto toImagePoint = [&](const float x, const float y) {
        return cv::Point(
            static_cast<int>(std::lround(offsetX + (x - minX) * scale)),
            static_cast<int>(std::lround(offsetY + (y - minY) * scale)));
    };

    cv::Mat image(kImageHeight, kImageWidth, CV_8UC3, cv::Scalar(241, 244, 247));
    cv::rectangle(
        image,
        cv::Rect(32, 32, kImageWidth - 64, kImageHeight - 64),
        cv::Scalar(255, 255, 255),
        cv::FILLED,
        cv::LINE_AA);

    cv::putText(
        image,
        "ATRACTORES",
        cv::Point(68, 78),
        cv::FONT_HERSHEY_DUPLEX,
        1.55,
        cv::Scalar(26, 31, 36),
        2,
        cv::LINE_AA);

    std::ostringstream details;
    details << graph.settings.width << 'x' << graph.settings.height
            << " | " << (graph.approximate ? "MODO APROX" : "MODO EXACTO")
            << " | PROC " << graph.processedSeeds
            << " | NOD " << graph.nodes.size()
            << " | EDGE " << graph.edges.size();
    cv::putText(
        image,
        details.str(),
        cv::Point(68, 112),
        cv::FONT_HERSHEY_SIMPLEX,
        0.72,
        cv::Scalar(82, 89, 96),
        2,
        cv::LINE_AA);

    for (const auto& [origin, destination] : graph.edges) {
        if (origin >= graph.nodes.size() || destination >= graph.nodes.size()) {
            continue;
        }

        const cv::Point from = toImagePoint(graph.nodes[origin].x, graph.nodes[origin].y);
        const cv::Point to = toImagePoint(graph.nodes[destination].x, graph.nodes[destination].y);
        cv::line(image, from, to, cv::Scalar(176, 132, 68), 2, cv::LINE_AA);
    }

    for (const AttractorGraphNode& node : graph.nodes) {
        const cv::Point center = toImagePoint(node.x, node.y);
        const int radius = node.cycle ? 8 : 5;
        const cv::Scalar fillColor = node.cycle ? cv::Scalar(46, 170, 255) : cv::Scalar(40, 112, 205);
        cv::circle(image, center, radius + 2, cv::Scalar(255, 255, 255), cv::FILLED, cv::LINE_AA);
        cv::circle(image, center, radius, fillColor, cv::FILLED, cv::LINE_AA);
        cv::circle(image, center, radius, cv::Scalar(23, 33, 49), 1, cv::LINE_AA);
    }

    if (!outputPath.parent_path().empty()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }
    if (!cv::imwrite(outputPath.string(), image)) {
        throw std::runtime_error("Unable to write PNG output file.");
    }
#endif
}
