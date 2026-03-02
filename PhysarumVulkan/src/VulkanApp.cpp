#include "FileDialog.h"
#include "VulkanApp.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <string_view>

namespace {

constexpr float kCanvasWidth = 500.0f;
constexpr float kCanvasHeight = 500.0f;
constexpr float kLogicalWindowWidth = 900.0f;
constexpr float kLogicalWindowHeight = 700.0f;
constexpr float kSidebarMinX = 520.0f;
constexpr float kSidebarInnerMinX = 540.0f;
constexpr float kSidebarInnerMaxX = 880.0f;
constexpr float kSidebarScrollAreaMinY = 96.0f;
constexpr float kSidebarScrollAreaMaxY = 696.0f;
constexpr float kSidebarScrollContentBottomY = 876.0f;
constexpr float kSidebarScrollStep = 28.0f;
constexpr float kScrollbarTrackMinX = 886.0f;
constexpr float kScrollbarTrackMaxX = 892.0f;
constexpr float kSidebarTitleY = 22.0f;
constexpr float kMenuButtonsMinY = 52.0f;
constexpr float kMenuButtonsMaxY = 90.0f;
constexpr float kStatusTextY = 100.0f;
constexpr float kStatesTitleY = 126.0f;
constexpr float kStateRowStartY = 154.0f;
constexpr float kStateRowHeight = 28.0f;
constexpr float kStateRowGap = 4.0f;
constexpr float kStateSwatchSize = 18.0f;
constexpr float kColorEditorTitleY = 462.0f;
constexpr float kSelectedStateTextY = 492.0f;
constexpr float kColorPreviewMinX = 786.0f;
constexpr float kColorPreviewMaxX = 872.0f;
constexpr float kColorPreviewMinY = 456.0f;
constexpr float kColorPreviewMaxY = 488.0f;
constexpr float kColorRowStartY = 530.0f;
constexpr float kColorRowHeight = 30.0f;
constexpr float kColorRowGap = 12.0f;
constexpr float kColorButtonWidth = 30.0f;
constexpr float kColorButtonMinX = 764.0f;
constexpr float kColorButtonMaxX = kColorButtonMinX + kColorButtonWidth;
constexpr float kColorButtonPlusMinX = 844.0f;
constexpr float kColorButtonPlusMaxX = kColorButtonPlusMinX + kColorButtonWidth;
constexpr uint8_t kColorAdjustStep = 8;
constexpr float kAttractorTextY = 646.0f;
constexpr float kAttractorButtonMinY = 662.0f;
constexpr float kAttractorButtonMaxY = 690.0f;
constexpr float kAttractorWidthMinusMinX = 596.0f;
constexpr float kAttractorWidthMinusMaxX = 626.0f;
constexpr float kAttractorWidthPlusMinX = 636.0f;
constexpr float kAttractorWidthPlusMaxX = 666.0f;
constexpr float kAttractorHeightMinusMinX = 754.0f;
constexpr float kAttractorHeightMinusMaxX = 784.0f;
constexpr float kAttractorHeightPlusMinX = 794.0f;
constexpr float kAttractorHeightPlusMaxX = 824.0f;
constexpr float kAttractorModeTextY = 718.0f;
constexpr float kAttractorRefineButtonMinY = 742.0f;
constexpr float kAttractorRefineButtonMaxY = 770.0f;
constexpr float kAttractorRefineButtonMinX = kSidebarInnerMinX;
constexpr float kAttractorRefineButtonMaxX = kSidebarInnerMaxX;
constexpr float kAttractorExportButtonMinY = 778.0f;
constexpr float kAttractorExportButtonMaxY = 806.0f;
constexpr float kAttractorExportButtonMinX = kSidebarInnerMinX;
constexpr float kAttractorExportButtonMaxX = kSidebarInnerMaxX;
constexpr float kAttractorExportPngButtonMinY = 814.0f;
constexpr float kAttractorExportPngButtonMaxY = 842.0f;
constexpr float kAttractorExportPngButtonMinX = kSidebarInnerMinX;
constexpr float kAttractorExportPngButtonMaxX = kSidebarInnerMaxX;
constexpr uint32_t kAttractorGraphVertexCapacity = 2'000'000U;
constexpr uint32_t kAttractorPreviewLegendVertexBudget = 16'384U;
constexpr int kAttractorPreviewWidth = 1440;
constexpr int kAttractorPreviewHeight = 960;
constexpr const char* kAttractorPreviewWindowName = "PhysarumVulkan | Atractores";
constexpr std::array<float, 4> kAttractorPreviewClearColor{{0.03f, 0.04f, 0.06f, 1.0f}};
constexpr std::array<float, 4> kAttractorPreviewPanelColor{{0.07f, 0.09f, 0.13f, 0.96f}};
constexpr std::array<float, 4> kAttractorPreviewTextColor{{0.90f, 0.93f, 0.97f, 1.0f}};
constexpr std::array<float, 4> kAttractorPreviewEdgeColor{{0.16f, 0.82f, 1.0f, 1.0f}};
constexpr std::array<float, 4> kAttractorPreviewNodeColor{{0.98f, 0.22f, 0.84f, 1.0f}};
constexpr std::array<float, 4> kAttractorPreviewCycleColor{{1.0f, 0.84f, 0.28f, 1.0f}};

bool shouldRenderSampledElement(const std::size_t index, const std::size_t totalCount, const std::size_t budgetCount) {
    if (budgetCount == 0 || totalCount == 0) {
        return false;
    }
    if (budgetCount >= totalCount) {
        return true;
    }
    return (index * budgetCount) / totalCount != ((index + 1) * budgetCount) / totalCount;
}

constexpr float pixelToNdcX(const float x) {
    return (x / kLogicalWindowWidth) * 2.0f - 1.0f;
}

constexpr float pixelToNdcY(const float y) {
    return (y / kLogicalWindowHeight) * 2.0f - 1.0f;
}

struct TexturedVertex {
    float position[2];
    float uv[2];

    static VkVertexInputBindingDescription bindingDescription() {
        VkVertexInputBindingDescription description{};
        description.binding = 0;
        description.stride = sizeof(TexturedVertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return description;
    }

    static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions() {
        return {{
            VkVertexInputAttributeDescription{
                0,
                0,
                VK_FORMAT_R32G32_SFLOAT,
                static_cast<uint32_t>(offsetof(TexturedVertex, position))
            },
            VkVertexInputAttributeDescription{
                1,
                0,
                VK_FORMAT_R32G32_SFLOAT,
                static_cast<uint32_t>(offsetof(TexturedVertex, uv))
            }
        }};
    }
};

struct SolidVertex {
    float position[2];

    static VkVertexInputBindingDescription bindingDescription() {
        VkVertexInputBindingDescription description{};
        description.binding = 0;
        description.stride = sizeof(SolidVertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return description;
    }

    static std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions() {
        return {{
            VkVertexInputAttributeDescription{
                0,
                0,
                VK_FORMAT_R32G32_SFLOAT,
                static_cast<uint32_t>(offsetof(SolidVertex, position))
            }
        }};
    }
};

struct RectPushConstants {
    float color[4];
};

struct LayoutRect {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
};

struct ClipRect {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
};

struct UiGeometryElement {
    LayoutRect rect{};
    SolidDrawRange draw{};
};

struct SolidGeometry {
    SolidDrawRange panel;
    SolidDrawRange sidebar;
    SolidDrawRange indicator;
    SolidDrawRange scrollbarTrack;
    SolidDrawRange scrollbarThumb;
    UiGeometryElement loadMapButton;
    UiGeometryElement attractorsButton;
    UiGeometryElement selectedColorPreview;
    std::array<UiGeometryElement, 9> stateButtons;
    std::array<UiGeometryElement, 9> stateSwatches;
    std::array<std::array<UiGeometryElement, 2>, 3> colorAdjustButtons;
    std::array<std::array<UiGeometryElement, 2>, 2> attractorAdjustButtons;
    UiGeometryElement attractorRefineButton;
    UiGeometryElement attractorExportButton;
    UiGeometryElement attractorExportPngButton;
    std::vector<SolidVertex> vertices;
};

constexpr std::array<TexturedVertex, 6> kSimulationQuadVertices{{
    {{pixelToNdcX(0.0f), pixelToNdcY(0.0f)}, {0.0f, 0.0f}},
    {{pixelToNdcX(kCanvasWidth), pixelToNdcY(0.0f)}, {1.0f, 0.0f}},
    {{pixelToNdcX(kCanvasWidth), pixelToNdcY(kCanvasHeight)}, {1.0f, 1.0f}},
    {{pixelToNdcX(0.0f), pixelToNdcY(0.0f)}, {0.0f, 0.0f}},
    {{pixelToNdcX(kCanvasWidth), pixelToNdcY(kCanvasHeight)}, {1.0f, 1.0f}},
    {{pixelToNdcX(0.0f), pixelToNdcY(kCanvasHeight)}, {0.0f, 1.0f}},
}};

constexpr std::array<float, 4> kPanelColor{{0.10f, 0.11f, 0.13f, 1.0f}};
constexpr std::array<float, 4> kSidebarColor{{0.07f, 0.08f, 0.10f, 1.0f}};
constexpr std::array<float, 4> kButtonColor{{0.17f, 0.20f, 0.24f, 1.0f}};
constexpr std::array<float, 4> kButtonHoverColor{{0.24f, 0.28f, 0.34f, 1.0f}};
constexpr std::array<float, 4> kButtonSelectedColor{{0.28f, 0.40f, 0.33f, 1.0f}};
constexpr std::array<float, 4> kButtonOutlineSoft{{0.12f, 0.14f, 0.17f, 1.0f}};
constexpr std::array<float, 4> kAttractorEdgeColor{{0.20f, 0.62f, 0.78f, 1.0f}};
constexpr std::array<float, 4> kAttractorNodeColor{{0.86f, 0.30f, 0.72f, 1.0f}};
constexpr std::array<float, 4> kAttractorCycleColor{{0.98f, 0.88f, 0.32f, 1.0f}};
constexpr std::array<float, 4> kClearColor{{0.03f, 0.03f, 0.04f, 1.0f}};
constexpr std::array<float, 4> kTextColor{{0.88f, 0.89f, 0.92f, 1.0f}};
constexpr float kIndicatorMinX = 10.0f;
constexpr float kIndicatorMinY = 570.0f;
constexpr float kIndicatorSize = 30.0f;
constexpr float kTextStartX = 54.0f;
constexpr float kStateTextStartY = 571.0f;
constexpr float kGenerationTextStartY = 609.0f;
constexpr float kStateTextPixelSize = 4.0f;
constexpr float kGenerationTextPixelSize = 3.0f;
constexpr uint32_t kOverlayTextVertexCapacity = 32768;

constexpr uint32_t packRgba8(const PhysarumSim::Rgba& color) {
    return static_cast<uint32_t>(color[0]) |
           (static_cast<uint32_t>(color[1]) << 8U) |
           (static_cast<uint32_t>(color[2]) << 16U) |
           (static_cast<uint32_t>(color[3]) << 24U);
}

constexpr std::array<std::string_view, 9> kStateLabels{{
    "LIBRE",
    "NUTR NO",
    "REPELENTE",
    "INICIO",
    "GEL CONT",
    "GEL COMP",
    "NUTR OK",
    "EXPANSION",
    "GEL SIN",
}};

SolidDrawRange appendRect(
    std::vector<SolidVertex>& vertices,
    const float minX,
    const float minY,
    const float maxX,
    const float maxY) {
    const uint32_t firstVertex = static_cast<uint32_t>(vertices.size());
    vertices.push_back({{pixelToNdcX(minX), pixelToNdcY(minY)}});
    vertices.push_back({{pixelToNdcX(maxX), pixelToNdcY(minY)}});
    vertices.push_back({{pixelToNdcX(maxX), pixelToNdcY(maxY)}});
    vertices.push_back({{pixelToNdcX(minX), pixelToNdcY(minY)}});
    vertices.push_back({{pixelToNdcX(maxX), pixelToNdcY(maxY)}});
    vertices.push_back({{pixelToNdcX(minX), pixelToNdcY(maxY)}});
    return {
        firstVertex,
        static_cast<uint32_t>(vertices.size()) - firstVertex
    };
}

float maxSidebarScrollOffset() {
    return std::max(0.0f, kSidebarScrollContentBottomY - kSidebarScrollAreaMaxY);
}

LayoutRect makeRect(const float minX, const float minY, const float maxX, const float maxY) {
    return LayoutRect{minX, minY, maxX, maxY};
}

LayoutRect translateRect(const LayoutRect rect, const float deltaY) {
    return LayoutRect{
        rect.minX,
        rect.minY + deltaY,
        rect.maxX,
        rect.maxY + deltaY
    };
}

LayoutRect clipLayoutRect(const LayoutRect rect, const ClipRect clipRect) {
    return LayoutRect{
        std::max(rect.minX, clipRect.minX),
        std::max(rect.minY, clipRect.minY),
        std::min(rect.maxX, clipRect.maxX),
        std::min(rect.maxY, clipRect.maxY)
    };
}

bool isVisibleRect(const LayoutRect rect) {
    return rect.maxX > rect.minX && rect.maxY > rect.minY;
}

UiGeometryElement appendUiRect(
    std::vector<SolidVertex>& vertices,
    const LayoutRect rect) {
    return UiGeometryElement{
        rect,
        appendRect(vertices, rect.minX, rect.minY, rect.maxX, rect.maxY)
    };
}

UiGeometryElement appendUiRectClipped(
    std::vector<SolidVertex>& vertices,
    const LayoutRect rect,
    const ClipRect clipRect) {
    const LayoutRect clippedRect = clipLayoutRect(rect, clipRect);
    if (!isVisibleRect(clippedRect)) {
        return UiGeometryElement{clippedRect, {static_cast<uint32_t>(vertices.size()), 0U}};
    }

    return UiGeometryElement{
        clippedRect,
        appendRect(vertices, clippedRect.minX, clippedRect.minY, clippedRect.maxX, clippedRect.maxY)
    };
}

std::array<uint8_t, 7> glyphPattern(const char glyph) {
    switch (glyph) {
        case '0':
            return {{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}};
        case '1':
            return {{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}};
        case '2':
            return {{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}};
        case '3':
            return {{0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}};
        case '4':
            return {{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}};
        case '5':
            return {{0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}};
        case '6':
            return {{0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}};
        case '7':
            return {{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}};
        case '8':
            return {{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}};
        case '9':
            return {{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}};
        case 'A':
            return {{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
        case 'B':
            return {{0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}};
        case 'C':
            return {{0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}};
        case 'D':
            return {{0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}};
        case 'E':
            return {{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}};
        case 'F':
            return {{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}};
        case 'G':
            return {{0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}};
        case 'H':
            return {{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
        case 'I':
            return {{0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}};
        case 'J':
            return {{0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}};
        case 'K':
            return {{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}};
        case 'L':
            return {{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}};
        case 'M':
            return {{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}};
        case 'N':
            return {{0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}};
        case 'O':
            return {{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}};
        case 'P':
            return {{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}};
        case 'Q':
            return {{0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}};
        case 'R':
            return {{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}};
        case 'S':
            return {{0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}};
        case 'T':
            return {{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}};
        case 'U':
            return {{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}};
        case 'V':
            return {{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}};
        case 'W':
            return {{0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}};
        case 'X':
            return {{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}};
        case 'Y':
            return {{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}};
        case 'Z':
            return {{0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}};
        case '+':
            return {{0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}};
        case '-':
            return {{0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}};
        case ' ':
            return {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
        default:
            return {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    }
}

void appendGlyph(
    std::vector<SolidVertex>& vertices,
    const char glyph,
    const float x,
    const float y,
    const float pixelSize) {
    const auto rows = glyphPattern(glyph);
    for (uint32_t row = 0; row < rows.size(); ++row) {
        for (uint32_t column = 0; column < 5; ++column) {
            const uint8_t mask = static_cast<uint8_t>(1U << (4U - column));
            if ((rows[row] & mask) == 0U) {
                continue;
            }

            const float minX = x + static_cast<float>(column) * pixelSize;
            const float minY = y + static_cast<float>(row) * pixelSize;
            appendRect(vertices, minX, minY, minX + pixelSize, minY + pixelSize);
        }
    }
}

void appendGlyphClipped(
    std::vector<SolidVertex>& vertices,
    const char glyph,
    const float x,
    const float y,
    const float pixelSize,
    const ClipRect clipRect) {
    const auto rows = glyphPattern(glyph);
    for (uint32_t row = 0; row < rows.size(); ++row) {
        for (uint32_t column = 0; column < 5; ++column) {
            const uint8_t mask = static_cast<uint8_t>(1U << (4U - column));
            if ((rows[row] & mask) == 0U) {
                continue;
            }

            const LayoutRect pixelRect{
                x + static_cast<float>(column) * pixelSize,
                y + static_cast<float>(row) * pixelSize,
                x + static_cast<float>(column + 1U) * pixelSize,
                y + static_cast<float>(row + 1U) * pixelSize
            };
            const LayoutRect clippedRect = clipLayoutRect(pixelRect, clipRect);
            if (!isVisibleRect(clippedRect)) {
                continue;
            }
            appendRect(vertices, clippedRect.minX, clippedRect.minY, clippedRect.maxX, clippedRect.maxY);
        }
    }
}

SolidDrawRange appendText(
    std::vector<SolidVertex>& vertices,
    const std::string_view text,
    const float startX,
    const float startY,
    const float pixelSize) {
    const uint32_t firstVertex = static_cast<uint32_t>(vertices.size());
    const float glyphAdvance = 6.0f * pixelSize;
    const float spaceAdvance = 4.0f * pixelSize;

    float cursorX = startX;
    for (const char glyph : text) {
        appendGlyph(vertices, glyph, cursorX, startY, pixelSize);
        cursorX += glyph == ' ' ? spaceAdvance : glyphAdvance;
    }

    return {
        firstVertex,
        static_cast<uint32_t>(vertices.size()) - firstVertex
    };
}

SolidDrawRange appendTextClipped(
    std::vector<SolidVertex>& vertices,
    const std::string_view text,
    const float startX,
    const float startY,
    const float pixelSize,
    const ClipRect clipRect) {
    const uint32_t firstVertex = static_cast<uint32_t>(vertices.size());
    const float glyphAdvance = 6.0f * pixelSize;
    const float spaceAdvance = 4.0f * pixelSize;

    float cursorX = startX;
    for (const char glyph : text) {
        appendGlyphClipped(vertices, glyph, cursorX, startY, pixelSize, clipRect);
        cursorX += glyph == ' ' ? spaceAdvance : glyphAdvance;
    }

    return {
        firstVertex,
        static_cast<uint32_t>(vertices.size()) - firstVertex
    };
}

SolidGeometry buildSolidGeometry(const float sidebarScrollOffset) {
    SolidGeometry geometry{};
    geometry.vertices.reserve(4096);
    const ClipRect scrollClipRect{
        kSidebarMinX,
        kSidebarScrollAreaMinY,
        kLogicalWindowWidth,
        kSidebarScrollAreaMaxY
    };

    geometry.panel = appendRect(geometry.vertices, 0.0f, kCanvasHeight, kCanvasWidth, kLogicalWindowHeight);
    geometry.sidebar = appendRect(geometry.vertices, kSidebarMinX, 0.0f, kLogicalWindowWidth, kLogicalWindowHeight);
    geometry.indicator = appendRect(
        geometry.vertices,
        kIndicatorMinX,
        kIndicatorMinY,
        kIndicatorMinX + kIndicatorSize,
        kIndicatorMinY + kIndicatorSize);
    geometry.loadMapButton = appendUiRect(
        geometry.vertices,
        makeRect(kSidebarInnerMinX, kMenuButtonsMinY, 700.0f, kMenuButtonsMaxY));
    geometry.attractorsButton = appendUiRect(
        geometry.vertices,
        makeRect(720.0f, kMenuButtonsMinY, kSidebarInnerMaxX, kMenuButtonsMaxY));
    geometry.scrollbarTrack = appendRect(
        geometry.vertices,
        kScrollbarTrackMinX,
        kSidebarScrollAreaMinY,
        kScrollbarTrackMaxX,
        kSidebarScrollAreaMaxY);

    const float scrollMax = maxSidebarScrollOffset();
    const float viewportHeight = kSidebarScrollAreaMaxY - kSidebarScrollAreaMinY;
    const float contentHeight = (kSidebarScrollContentBottomY - kSidebarScrollAreaMinY) + scrollMax;
    const float thumbHeight = scrollMax <= 0.0f
        ? viewportHeight
        : std::clamp(viewportHeight * (viewportHeight / contentHeight), 36.0f, viewportHeight);
    const float thumbTravel = std::max(0.0f, viewportHeight - thumbHeight);
    const float thumbOffset = scrollMax <= 0.0f ? 0.0f : (sidebarScrollOffset / scrollMax) * thumbTravel;
    geometry.scrollbarThumb = appendRect(
        geometry.vertices,
        kScrollbarTrackMinX,
        kSidebarScrollAreaMinY + thumbOffset,
        kScrollbarTrackMaxX,
        kSidebarScrollAreaMinY + thumbOffset + thumbHeight);

    geometry.selectedColorPreview = appendUiRectClipped(
        geometry.vertices,
        translateRect(makeRect(kColorPreviewMinX, kColorPreviewMinY, kColorPreviewMaxX, kColorPreviewMaxY), -sidebarScrollOffset),
        scrollClipRect);

    for (std::size_t index = 0; index < geometry.stateButtons.size(); ++index) {
        const float rowMinY = kStateRowStartY + static_cast<float>(index) * (kStateRowHeight + kStateRowGap);
        const float rowMaxY = rowMinY + kStateRowHeight;
        geometry.stateButtons[index] = appendUiRectClipped(
            geometry.vertices,
            translateRect(makeRect(kSidebarInnerMinX, rowMinY, kSidebarInnerMaxX, rowMaxY), -sidebarScrollOffset),
            scrollClipRect);
        geometry.stateSwatches[index] = appendUiRectClipped(
            geometry.vertices,
            translateRect(makeRect(
                kSidebarInnerMinX + 8.0f,
                rowMinY + 5.0f,
                kSidebarInnerMinX + 8.0f + kStateSwatchSize,
                rowMinY + 5.0f + kStateSwatchSize), -sidebarScrollOffset),
            scrollClipRect);
    }

    for (std::size_t channel = 0; channel < geometry.colorAdjustButtons.size(); ++channel) {
        const float rowMinY = kColorRowStartY + static_cast<float>(channel) * (kColorRowHeight + kColorRowGap);
        const float rowMaxY = rowMinY + kColorRowHeight;
        geometry.colorAdjustButtons[channel][0] = appendUiRectClipped(
            geometry.vertices,
            translateRect(makeRect(kColorButtonMinX, rowMinY, kColorButtonMaxX, rowMaxY), -sidebarScrollOffset),
            scrollClipRect);
        geometry.colorAdjustButtons[channel][1] = appendUiRectClipped(
            geometry.vertices,
            translateRect(makeRect(kColorButtonPlusMinX, rowMinY, kColorButtonPlusMaxX, rowMaxY), -sidebarScrollOffset),
            scrollClipRect);
    }

    geometry.attractorAdjustButtons[0][0] = appendUiRectClipped(
        geometry.vertices,
        translateRect(makeRect(kAttractorWidthMinusMinX, kAttractorButtonMinY, kAttractorWidthMinusMaxX, kAttractorButtonMaxY), -sidebarScrollOffset),
        scrollClipRect);
    geometry.attractorAdjustButtons[0][1] = appendUiRectClipped(
        geometry.vertices,
        translateRect(makeRect(kAttractorWidthPlusMinX, kAttractorButtonMinY, kAttractorWidthPlusMaxX, kAttractorButtonMaxY), -sidebarScrollOffset),
        scrollClipRect);
    geometry.attractorAdjustButtons[1][0] = appendUiRectClipped(
        geometry.vertices,
        translateRect(makeRect(kAttractorHeightMinusMinX, kAttractorButtonMinY, kAttractorHeightMinusMaxX, kAttractorButtonMaxY), -sidebarScrollOffset),
        scrollClipRect);
    geometry.attractorAdjustButtons[1][1] = appendUiRectClipped(
        geometry.vertices,
        translateRect(makeRect(kAttractorHeightPlusMinX, kAttractorButtonMinY, kAttractorHeightPlusMaxX, kAttractorButtonMaxY), -sidebarScrollOffset),
        scrollClipRect);
    geometry.attractorRefineButton = appendUiRectClipped(
        geometry.vertices,
        translateRect(
            makeRect(
                kAttractorRefineButtonMinX,
                kAttractorRefineButtonMinY,
                kAttractorRefineButtonMaxX,
                kAttractorRefineButtonMaxY),
            -sidebarScrollOffset),
        scrollClipRect);
    geometry.attractorExportButton = appendUiRectClipped(
        geometry.vertices,
        translateRect(
            makeRect(
                kAttractorExportButtonMinX,
                kAttractorExportButtonMinY,
                kAttractorExportButtonMaxX,
                kAttractorExportButtonMaxY),
            -sidebarScrollOffset),
        scrollClipRect);
    geometry.attractorExportPngButton = appendUiRectClipped(
        geometry.vertices,
        translateRect(
            makeRect(
                kAttractorExportPngButtonMinX,
                kAttractorExportPngButtonMinY,
                kAttractorExportPngButtonMaxX,
                kAttractorExportPngButtonMaxY),
            -sidebarScrollOffset),
        scrollClipRect);

    return geometry;
}

void throwIfFailed(const VkResult result, const std::string_view action) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(std::string(action) + " (VkResult=" + std::to_string(result) + ")");
    }
}

VkResult createDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* messenger) {
    const auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (function == nullptr) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    return function(instance, createInfo, allocator, messenger);
}

void destroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    const VkAllocationCallbacks* allocator) {
    const auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (function != nullptr) {
        function(instance, messenger, allocator);
    }
}

}  // namespace

VulkanApp::VulkanApp(const GridSize initialGridSize)
    : model_(initialGridSize),
      viewModel_(),
      controller_(model_, viewModel_),
      simulation_(model_.simulation()),
      requestedGridSize_(model_.requestedGridSize()),
      selectedState_(viewModel_.selectedState()),
      sidebarScrollOffset_(viewModel_.sidebarScrollOffset()),
      menuStatus_(viewModel_.menuStatus()),
      showingAttractorGraph_(viewModel_.showingAttractorGraph()),
      attractorSettings_(viewModel_.attractorSettings()),
      attractorProgress_(viewModel_.attractorProgress()),
      latestAttractorGraph_(viewModel_.latestAttractorGraph()),
      attractorComputeStatus_(viewModel_.attractorComputeStatus()) {
}

void VulkanApp::run() {
    try {
        initWindow();
        initVulkan();
        mainLoop();
    } catch (...) {
        cleanup();
        throw;
    }

    cleanup();
}

void VulkanApp::initWindow() {
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window_ = glfwCreateWindow(
        static_cast<int>(kWindowWidth),
        static_cast<int>(kWindowHeight),
        "PhysarumVulkan",
        nullptr,
        nullptr);
    if (window_ == nullptr) {
        throw std::runtime_error("Failed to create GLFW window.");
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    panCursor_ = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    refreshWindowTitle();
}

void VulkanApp::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    validateGridSize(requestedGridSize_);
    createLogicalDevice();
    createCommandPool();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createComputeDescriptorSetLayout();
    createPipelineLayouts();
    createGraphicsPipelines();
    createComputePipeline();
    createFramebuffers();
    createVertexBuffers();
    createOverlayTextBuffer();
    createAttractorGraphBuffer();
    createSimulationBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createStagingBuffers();
    createCommandBuffers();
    createSyncObjects();

    simulationGpuEnabled_ = simulationGraphicsQueueComputeCapable_ && computePipeline_ != VK_NULL_HANDLE;

    attractorCompute_.initialize(AttractorCompute::CreateInfo{
        physicalDevice_,
        device_,
        queueFamilyIndices_.computeFamily.value_or(queueFamilyIndices_.graphicsFamily.value()),
        computeQueue_ != VK_NULL_HANDLE ? computeQueue_ : graphicsQueue_,
        shaderPath("attractor.comp.spv"),
        &queueSubmitMutex_
    });
    controller_.setAttractorBackendStatus(attractorCompute_.status());
    std::cout << "Attractor backend: " << attractorComputeStatus_ << '\n';
    std::cout << "Simulation backend: " << (simulationGpuEnabled_ ? "SIM GPU" : "SIM CPU") << '\n';

    logGridConfiguration();
}

void VulkanApp::mainLoop() {
    controller_.initializeSimulationClock(std::chrono::steady_clock::now());
    auto nextFrameDeadline = std::chrono::steady_clock::now();

    while (window_ != nullptr && glfwWindowShouldClose(window_) == GLFW_FALSE) {
        glfwPollEvents();
        processInput();
        updateSimulation();
        updateAttractorState();
        refreshWindowTitle();
        drawFrame();
        updateAttractorPreviewWindow();

        if (!model_.play()) {
            nextFrameDeadline += kTargetFrameTime;
            std::this_thread::sleep_until(nextFrameDeadline);

            const auto now = std::chrono::steady_clock::now();
            if (now > nextFrameDeadline + kTargetFrameTime) {
                nextFrameDeadline = now;
            }
        } else {
            nextFrameDeadline = std::chrono::steady_clock::now();
        }
    }

    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
}

void VulkanApp::cleanup() {
    controller_.shutdown();
    closeAttractorPreviewWindow();

    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    attractorCompute_.cleanup();

    cleanupSwapChain();

    for (BufferAllocation& stagingBuffer : stagingBuffers_) {
        destroyBuffer(stagingBuffer);
    }

    destroyBuffer(texturedVertexBuffer_);
    destroyBuffer(solidVertexBuffer_);
    destroyBuffer(overlayTextVertexBuffer_);
    destroyBuffer(attractorGraphVertexBuffer_);
    destroySimulationBuffers();

    if (descriptorPool_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
        descriptorSets_ = {};
        computeDescriptorSets_ = {};
    }

    if (solidPipelineLayout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, solidPipelineLayout_, nullptr);
        solidPipelineLayout_ = VK_NULL_HANDLE;
    }

    if (computePipelineLayout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, computePipelineLayout_, nullptr);
        computePipelineLayout_ = VK_NULL_HANDLE;
    }

    if (texturedPipelineLayout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, texturedPipelineLayout_, nullptr);
        texturedPipelineLayout_ = VK_NULL_HANDLE;
    }

    if (computePipeline_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, computePipeline_, nullptr);
        computePipeline_ = VK_NULL_HANDLE;
    }

    if (descriptorSetLayout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }

    if (computeDescriptorSetLayout_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, computeDescriptorSetLayout_, nullptr);
        computeDescriptorSetLayout_ = VK_NULL_HANDLE;
    }

    for (std::size_t index = 0; index < kMaxFramesInFlight; ++index) {
        if (imageAvailableSemaphores_[index] != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, imageAvailableSemaphores_[index], nullptr);
            imageAvailableSemaphores_[index] = VK_NULL_HANDLE;
        }
        if (renderFinishedSemaphores_[index] != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, renderFinishedSemaphores_[index], nullptr);
            renderFinishedSemaphores_[index] = VK_NULL_HANDLE;
        }
        if (inFlightFences_[index] != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroyFence(device_, inFlightFences_[index], nullptr);
            inFlightFences_[index] = VK_NULL_HANDLE;
        }
    }

    if (commandPool_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }

    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (debugMessenger_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
        destroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
        debugMessenger_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }

    if (window_ != nullptr) {
        if (panCursor_ != nullptr) {
            glfwDestroyCursor(panCursor_);
            panCursor_ = nullptr;
        }
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }

    glfwTerminate();
}

void VulkanApp::processInput() {
    const auto handleEdge = [this](const int key, const auto& action) {
        const bool isPressed = glfwGetKey(window_, key) == GLFW_PRESS;
        if (isPressed && !keyPressed_[key]) {
            action();
        }
        keyPressed_[key] = isPressed;
    };

    handleEdge(GLFW_KEY_ESCAPE, [this]() {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    });

    handleEdge(GLFW_KEY_ENTER, [this]() {
        controller_.togglePlayback();
    });
    handleEdge(GLFW_KEY_KP_ENTER, [this]() {
        controller_.togglePlayback();
    });

    handleEdge(GLFW_KEY_1, [this]() { controller_.selectState(0); });
    handleEdge(GLFW_KEY_2, [this]() { controller_.selectState(1); });
    handleEdge(GLFW_KEY_3, [this]() { controller_.selectState(2); });
    handleEdge(GLFW_KEY_4, [this]() { controller_.selectState(3); });
    handleEdge(GLFW_KEY_5, [this]() { controller_.selectState(4); });
    handleEdge(GLFW_KEY_6, [this]() { controller_.selectState(5); });
    handleEdge(GLFW_KEY_7, [this]() { controller_.selectState(6); });
    handleEdge(GLFW_KEY_8, [this]() { controller_.selectState(7); });
    handleEdge(GLFW_KEY_9, [this]() { controller_.selectState(8); });

    handleEdge(GLFW_KEY_KP_1, [this]() { controller_.selectState(0); });
    handleEdge(GLFW_KEY_KP_2, [this]() { controller_.selectState(1); });
    handleEdge(GLFW_KEY_KP_3, [this]() { controller_.selectState(2); });
    handleEdge(GLFW_KEY_KP_4, [this]() { controller_.selectState(3); });
    handleEdge(GLFW_KEY_KP_5, [this]() { controller_.selectState(4); });
    handleEdge(GLFW_KEY_KP_6, [this]() { controller_.selectState(5); });
    handleEdge(GLFW_KEY_KP_7, [this]() { controller_.selectState(6); });
    handleEdge(GLFW_KEY_KP_8, [this]() { controller_.selectState(7); });
    handleEdge(GLFW_KEY_KP_9, [this]() { controller_.selectState(8); });

    handleEdge(GLFW_KEY_F1, [this]() { requestGridResize({200, 200}); });
    handleEdge(GLFW_KEY_F2, [this]() { requestGridResize({512, 512}); });
    handleEdge(GLFW_KEY_F3, [this]() { requestGridResize({1024, 1024}); });
    handleEdge(GLFW_KEY_F4, [this]() { requestGridResize({4000, 4000}); });
    handleEdge(GLFW_KEY_R, [this]() { resetView(); });

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window_, &mouseX, &mouseY);
    const auto [logicalMouseX, logicalMouseY] = screenToLogical(mouseX, mouseY);
    mouseLogicalX_ = logicalMouseX;
    mouseLogicalY_ = logicalMouseY;
    updateCursorFeedback(mouseX, mouseY);

    const bool panRequested = updatePan();
    const bool viewMovingBeforeInput =
        panRequested ||
        std::abs(panVelocityX_) > kPanVelocityEpsilon ||
        std::abs(panVelocityY_) > kPanVelocityEpsilon;
    if (viewMovingBeforeInput) {
        pendingZoomDelta_ = 0.0;
    } else {
        applyPendingZoom();
    }
    updateViewMotion();

    const bool viewInMotion =
        panRequested ||
        std::abs(panVelocityX_) > kPanVelocityEpsilon ||
        std::abs(panVelocityY_) > kPanVelocityEpsilon;
    const bool leftPressed = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool leftClickStarted = leftPressed && !leftMousePressed_;
    if (!leftPressed) {
        uiMouseCapture_ = false;
    }

    if (viewInMotion) {
        leftMousePressed_ = leftPressed;
        return;
    }

    const bool insideSidebar =
        logicalMouseX >= kSidebarMinX &&
        logicalMouseX <= kLogicalWindowWidth &&
        logicalMouseY >= 0.0f &&
        logicalMouseY <= kLogicalWindowHeight;

    if (leftClickStarted && insideSidebar) {
        uiMouseCapture_ = true;
        handleSidebarClick(logicalMouseX, logicalMouseY);
        leftMousePressed_ = leftPressed;
        return;
    }

    if (!leftPressed || uiMouseCapture_ || !isInsideSimulationArea(mouseX, mouseY)) {
        leftMousePressed_ = leftPressed;
        return;
    }

    const auto [cellX, cellY] = screenToCell(mouseX, mouseY);
    if (simulationGpuEnabled_) {
        simulation_.overwriteCellState(cellX, cellY, selectedState_);
        gpuStepSubmitted_ = false;
        gpuPhysarumLastCells_ = 0;
        gpuMinimumPhysarumCells_ = 0;
        gpuMinimumCheck_ = 0;
    } else {
        controller_.paintSelectedState(cellX, cellY);
    }
    leftMousePressed_ = leftPressed;
}

void VulkanApp::updateSimulation() {
    const auto now = std::chrono::steady_clock::now();
    if (!simulationGpuEnabled_) {
        controller_.advanceSimulation(now, targetSimulationInterval());
        return;
    }

    consumeGpuSimulationResults();
    pendingGpuSimulationStep_ = false;
    if (!model_.shouldAdvanceSimulation(now, targetSimulationInterval())) {
        return;
    }

    pendingGpuSimulationStep_ = true;
    model_.commitSimulationAdvance(now, false);
}

void VulkanApp::updateAttractorState() {
    const AttractorUpdate update = controller_.pollAttractorUpdates();
    if (update.graph != nullptr) {
        try {
            if (showingAttractorGraph_) {
                renderAttractorPreview(*update.graph);
            }
        } catch (const std::exception&) {
            attractorGraphVertexCount_ = 0;
            attractorGraphDraws_ = {};
            closeAttractorPreviewWindow();
            controller_.setAttractorError();
            return;
        }
    }
}

void VulkanApp::exportAttractorSvg() {
    if (!controller_.hasExportableAttractorGraph()) {
        controller_.setMenuStatus(MenuStatus::SvgExportError);
        return;
    }

    const SaveFileDialogResult dialogResult = pickSaveSvgFile();
    if (!dialogResult.available) {
        controller_.setMenuStatus(MenuStatus::DialogUnavailable);
        return;
    }
    if (!dialogResult.path.has_value()) {
        controller_.setMenuStatus(MenuStatus::Ready);
        return;
    }

    try {
        attractorExporter_.writeSvg(dialogResult.path.value(), controller_.latestAttractorGraph().value());
        controller_.setMenuStatus(MenuStatus::SvgExported);
    } catch (const std::exception&) {
        controller_.setMenuStatus(MenuStatus::SvgExportError);
    }
}

void VulkanApp::exportAttractorPng() {
    if (!controller_.hasExportableAttractorGraph()) {
        controller_.setMenuStatus(MenuStatus::PngExportError);
        return;
    }

    if (!AttractorGraphExporter::pngSupported()) {
        controller_.setMenuStatus(MenuStatus::OpenCvUnavailable);
        return;
    }

    const SaveFileDialogResult dialogResult = pickSavePngFile();
    if (!dialogResult.available) {
        controller_.setMenuStatus(MenuStatus::DialogUnavailable);
        return;
    }
    if (!dialogResult.path.has_value()) {
        controller_.setMenuStatus(MenuStatus::Ready);
        return;
    }

    try {
        attractorExporter_.writePng(dialogResult.path.value(), controller_.latestAttractorGraph().value());
        controller_.setMenuStatus(MenuStatus::PngExported);
    } catch (const std::exception&) {
        controller_.setMenuStatus(MenuStatus::PngExportError);
    }
}

BatchSuccessorEvaluator VulkanApp::currentSuccessorEvaluator() {
    if (!attractorCompute_.available()) {
        return {};
    }

    return [this](
               const AttractorSettings& settings,
               const std::vector<AttractorStateBlock>& inputStates,
               std::vector<AttractorStateBlock>& outputStates) {
        attractorCompute_.evaluateSuccessors(settings, inputStates, outputStates);
    };
}

std::chrono::milliseconds VulkanApp::targetSimulationInterval() const {
    if (simulationGpuEnabled_) {
        return std::chrono::milliseconds(0);
    }

    const GridSize size = requestedGridSize_;
    const uint64_t area = static_cast<uint64_t>(size.w) * static_cast<uint64_t>(size.h);

    if (area >= 8'000'000ULL) {
        return std::chrono::milliseconds(120);
    }
    if (area >= 2'000'000ULL) {
        return std::chrono::milliseconds(48);
    }
    if (area >= 500'000ULL) {
        return std::chrono::milliseconds(16);
    }
    return std::chrono::milliseconds(4);
}

VulkanApp::ScreenRect VulkanApp::simulationViewportRect() const {
    if (window_ == nullptr) {
        return {};
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window_, &windowWidth, &windowHeight);

    const double logicalSimulationWidth =
        static_cast<double>(kSimulationViewportSize) / static_cast<double>(kWindowWidth);
    const double logicalSimulationHeight =
        static_cast<double>(kSimulationViewportSize) / static_cast<double>(kWindowHeight);

    ScreenRect rect{};
    rect.width = static_cast<double>(windowWidth) * logicalSimulationWidth;
    rect.height = static_cast<double>(windowHeight) * logicalSimulationHeight;
    return rect;
}

bool VulkanApp::isInsideSimulationArea(const double mouseX, const double mouseY) const {
    const ScreenRect viewport = simulationViewportRect();
    return viewport.width > 0.0 &&
           viewport.height > 0.0 &&
           mouseX >= viewport.x &&
           mouseY >= viewport.y &&
           mouseX < viewport.x + viewport.width &&
           mouseY < viewport.y + viewport.height;
}

void VulkanApp::resetView() {
    zoom_ = 1.0f;
    viewCenterX_ = 0.5f;
    viewCenterY_ = 0.5f;
    panVelocityX_ = 0.0f;
    panVelocityY_ = 0.0f;
}

void VulkanApp::clampView() {
    const float visibleWidth = 1.0f / zoom_;
    const float visibleHeight = 1.0f / zoom_;
    const float halfWidth = visibleWidth * 0.5f;
    const float halfHeight = visibleHeight * 0.5f;

    viewCenterX_ = std::clamp(viewCenterX_, halfWidth, 1.0f - halfWidth);
    viewCenterY_ = std::clamp(viewCenterY_, halfHeight, 1.0f - halfHeight);
}

void VulkanApp::applyPendingZoom() {
    if (pendingZoomDelta_ == 0.0) {
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window_, &mouseX, &mouseY);
    if (!isInsideSimulationArea(mouseX, mouseY)) {
        pendingZoomDelta_ = 0.0;
        return;
    }

    const float zoomMultiplier = std::pow(1.2f, static_cast<float>(pendingZoomDelta_));
    pendingZoomDelta_ = 0.0;
    zoomAtCursor(mouseX, mouseY, zoomMultiplier);
}

void VulkanApp::updateCursorFeedback(const double mouseX, const double mouseY) {
    if (window_ == nullptr) {
        return;
    }

    const bool ctrlPressed =
        glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    const bool showPanCursor =
        panActive_ || (ctrlPressed && isInsideSimulationArea(mouseX, mouseY));

    if (showPanCursor && panCursor_ != nullptr) {
        glfwSetCursor(window_, panCursor_);
        return;
    }

    glfwSetCursor(window_, nullptr);
}

void VulkanApp::zoomAtCursor(const double mouseX, const double mouseY, const float zoomMultiplier) {
    const ScreenRect viewport = simulationViewportRect();
    if (viewport.width <= 0.0 || viewport.height <= 0.0) {
        return;
    }

    const float beforeSpan = 1.0f / zoom_;
    const float cursorU = static_cast<float>(
        std::clamp((mouseX - viewport.x) / viewport.width, 0.0, 1.0));
    const float cursorV = static_cast<float>(
        std::clamp((mouseY - viewport.y) / viewport.height, 0.0, 1.0));
    const float beforeMinX = viewCenterX_ - beforeSpan * 0.5f;
    const float beforeMinY = viewCenterY_ - beforeSpan * 0.5f;
    const float focusU = beforeMinX + beforeSpan * cursorU;
    const float focusV = beforeMinY + beforeSpan * cursorV;

    zoom_ = std::clamp(zoom_ * zoomMultiplier, 1.0f, 64.0f);

    const float afterSpan = 1.0f / zoom_;
    viewCenterX_ = focusU + (0.5f - cursorU) * afterSpan;
    viewCenterY_ = focusV + (0.5f - cursorV) * afterSpan;

    clampView();
}

bool VulkanApp::updatePan() {
    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window_, &mouseX, &mouseY);

    const bool middlePressed = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const bool leftPressed = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool ctrlPressed =
        glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    const bool panRequested = middlePressed || (ctrlPressed && leftPressed);

    if (!panRequested || !isInsideSimulationArea(mouseX, mouseY)) {
        panActive_ = false;
        return false;
    }

    if (!panActive_) {
        panActive_ = true;
        lastPanMouseX_ = mouseX;
        lastPanMouseY_ = mouseY;
        panVelocityX_ = 0.0f;
        panVelocityY_ = 0.0f;
        return true;
    }

    const double deltaX = mouseX - lastPanMouseX_;
    const double deltaY = mouseY - lastPanMouseY_;
    lastPanMouseX_ = mouseX;
    lastPanMouseY_ = mouseY;

    const ScreenRect viewport = simulationViewportRect();
    if (viewport.width <= 0.0 || viewport.height <= 0.0) {
        return false;
    }

    const float visibleSpan = 1.0f / zoom_;
    const float desiredVelocityX = -static_cast<float>(deltaX / viewport.width) * visibleSpan;
    const float desiredVelocityY = -static_cast<float>(deltaY / viewport.height) * visibleSpan;
    panVelocityX_ =
        panVelocityX_ * kPanBlendFactor + desiredVelocityX * (1.0f - kPanBlendFactor);
    panVelocityY_ =
        panVelocityY_ * kPanBlendFactor + desiredVelocityY * (1.0f - kPanBlendFactor);
    return true;
}

void VulkanApp::updateViewMotion() {
    if (std::abs(panVelocityX_) <= kPanVelocityEpsilon) {
        panVelocityX_ = 0.0f;
    }
    if (std::abs(panVelocityY_) <= kPanVelocityEpsilon) {
        panVelocityY_ = 0.0f;
    }
    if (panVelocityX_ == 0.0f && panVelocityY_ == 0.0f) {
        return;
    }

    const float unclampedCenterX = viewCenterX_ + panVelocityX_;
    const float unclampedCenterY = viewCenterY_ + panVelocityY_;
    viewCenterX_ = unclampedCenterX;
    viewCenterY_ = unclampedCenterY;
    clampView();

    if (viewCenterX_ != unclampedCenterX) {
        panVelocityX_ = 0.0f;
    }
    if (viewCenterY_ != unclampedCenterY) {
        panVelocityY_ = 0.0f;
    }

    if (!panActive_) {
        panVelocityX_ *= kPanInertiaDamping;
        panVelocityY_ *= kPanInertiaDamping;
    }
}

VulkanApp::QuadPushConstants VulkanApp::currentQuadPushConstants() const {
    const float visibleSpan = 1.0f / zoom_;
    const float halfSpan = visibleSpan * 0.5f;
    const GridSize size = simulation_.gridSize();

    QuadPushConstants push{};
    push.uvMin[0] = viewCenterX_ - halfSpan;
    push.uvMin[1] = viewCenterY_ - halfSpan;
    push.uvMax[0] = viewCenterX_ + halfSpan;
    push.uvMax[1] = viewCenterY_ + halfSpan;
    push.gridWidth = size.w;
    push.gridHeight = size.h;
    return push;
}

std::pair<float, float> VulkanApp::screenToLogical(const double mouseX, const double mouseY) const {
    if (window_ == nullptr) {
        return {0.0f, 0.0f};
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window_, &windowWidth, &windowHeight);
    if (windowWidth <= 0 || windowHeight <= 0) {
        return {0.0f, 0.0f};
    }

    const float logicalX =
        static_cast<float>(mouseX / static_cast<double>(windowWidth) * static_cast<double>(kLogicalWindowWidth));
    const float logicalY =
        static_cast<float>(mouseY / static_cast<double>(windowHeight) * static_cast<double>(kLogicalWindowHeight));
    return {logicalX, logicalY};
}

std::pair<uint32_t, uint32_t> VulkanApp::screenToCell(const double mouseX, const double mouseY) const {
    const QuadPushConstants view = currentQuadPushConstants();
    const ScreenRect viewport = simulationViewportRect();
    if (viewport.width <= 0.0 || viewport.height <= 0.0) {
        return {0, 0};
    }

    const double normalizedX = std::clamp((mouseX - viewport.x) / viewport.width, 0.0, 1.0);
    const double normalizedY = std::clamp((mouseY - viewport.y) / viewport.height, 0.0, 1.0);
    const double u = std::clamp(
        static_cast<double>(view.uvMin[0]) + normalizedX * static_cast<double>(view.uvMax[0] - view.uvMin[0]),
        0.0,
        0.999999);
    const double v = std::clamp(
        static_cast<double>(view.uvMin[1]) + normalizedY * static_cast<double>(view.uvMax[1] - view.uvMin[1]),
        0.0,
        0.999999);

    const GridSize gridSize = simulation_.gridSize();
    const uint32_t cellX = std::min<uint32_t>(static_cast<uint32_t>(u * static_cast<double>(gridSize.w)), gridSize.w - 1U);
    const uint32_t cellY = std::min<uint32_t>(static_cast<uint32_t>(v * static_cast<double>(gridSize.h)), gridSize.h - 1U);
    return {cellX, cellY};
}

bool VulkanApp::isInsideSidebarScrollableArea(const float logicalX, const float logicalY) const {
    return logicalX >= kSidebarMinX &&
           logicalX <= kLogicalWindowWidth &&
           logicalY >= kSidebarScrollAreaMinY &&
           logicalY <= kSidebarScrollAreaMaxY;
}

float VulkanApp::maxSidebarScrollOffset() const {
    return ::maxSidebarScrollOffset();
}

void VulkanApp::nudgeSidebarScroll(const float delta) {
    const float nextOffset = std::clamp(sidebarScrollOffset_ + delta, 0.0f, maxSidebarScrollOffset());
    if (std::abs(nextOffset - sidebarScrollOffset_) < 0.01f) {
        return;
    }

    sidebarScrollOffset_ = nextOffset;
    if (device_ != VK_NULL_HANDLE) {
        rebuildSolidUiBuffer();
    }
}

void VulkanApp::handleSidebarClick(const float logicalX, const float logicalY) {
    const auto insideRect = [logicalX, logicalY](const ScreenRect& rect) {
        return logicalX >= static_cast<float>(rect.x) &&
               logicalY >= static_cast<float>(rect.y) &&
               logicalX <= static_cast<float>(rect.x + rect.width) &&
               logicalY <= static_cast<float>(rect.y + rect.height);
    };

    if (insideRect(loadMapButton_.rect)) {
        const ImageFileDialogResult dialogResult = pickImageFile();
        if (!dialogResult.available) {
            controller_.setMenuStatus(MenuStatus::DialogUnavailable);
            return;
        }
        if (!dialogResult.path.has_value()) {
            controller_.setMenuStatus(MenuStatus::Ready);
            return;
        }

        closeAttractorPreviewWindow();
        controller_.loadMapFromImage(dialogResult.path.value(), std::chrono::steady_clock::now());
        gpuStepSubmitted_ = false;
        gpuPhysarumLastCells_ = 0;
        gpuMinimumPhysarumCells_ = 0;
        gpuMinimumCheck_ = 0;
        return;
    }

    if (insideRect(attractorsButton_.rect)) {
        const AttractorPreviewRequest request = controller_.toggleAttractorGraph(currentSuccessorEvaluator());
        if (request.closePreview) {
            closeAttractorPreviewWindow();
        }
        if (request.clearRenderedGraph) {
            attractorGraphVertexCount_ = 0;
            attractorGraphDraws_ = {};
        }
        if (request.resetPreviewBounds) {
            resetAttractorPreviewBounds();
        }
        if (request.openPreview) {
            renderAttractorStatusPreview(request.statusText);
        }
        return;
    }

    if (insideRect(attractorExportButton_.rect)) {
        exportAttractorSvg();
        return;
    }

    if (insideRect(attractorExportPngButton_.rect)) {
        exportAttractorPng();
        return;
    }

    if (insideRect(attractorRefineButton_.rect)) {
        const AttractorPreviewRequest request = controller_.refineAttractorGraph(currentSuccessorEvaluator());
        if (request.closePreview) {
            closeAttractorPreviewWindow();
        }
        if (request.clearRenderedGraph) {
            attractorGraphVertexCount_ = 0;
            attractorGraphDraws_ = {};
        }
        if (request.resetPreviewBounds) {
            resetAttractorPreviewBounds();
        }
        if (request.openPreview) {
            renderAttractorStatusPreview(request.statusText);
        }
        return;
    }

    for (std::size_t index = 0; index < stateButtons_.size(); ++index) {
        if (insideRect(stateButtons_[index].rect)) {
            controller_.selectState(static_cast<uint8_t>(index));
            controller_.setMenuStatus(MenuStatus::Ready);
            return;
        }
    }

    for (std::size_t channel = 0; channel < colorAdjustButtons_.size(); ++channel) {
        if (insideRect(colorAdjustButtons_[channel].decrement.rect)) {
            nudgeSelectedStateColor(channel, -static_cast<int>(kColorAdjustStep));
            return;
        }
        if (insideRect(colorAdjustButtons_[channel].increment.rect)) {
            nudgeSelectedStateColor(channel, static_cast<int>(kColorAdjustStep));
            return;
        }
    }

    if (insideRect(attractorWidthButtons_.decrement.rect)) {
        nudgeAttractorDimension(true, -1);
        return;
    }
    if (insideRect(attractorWidthButtons_.increment.rect)) {
        nudgeAttractorDimension(true, 1);
        return;
    }
    if (insideRect(attractorHeightButtons_.decrement.rect)) {
        nudgeAttractorDimension(false, -1);
        return;
    }
    if (insideRect(attractorHeightButtons_.increment.rect)) {
        nudgeAttractorDimension(false, 1);
        return;
    }
}

void VulkanApp::nudgeSelectedStateColor(const std::size_t channel, const int delta) {
    controller_.nudgeSelectedStateColor(channel, delta);
    simulationPaletteDirty_ = true;
}

void VulkanApp::nudgeAttractorDimension(const bool adjustWidth, const int delta) {
    if (!controller_.nudgeAttractorDimension(adjustWidth, delta)) {
        return;
    }

    attractorGraphVertexCount_ = 0;
    attractorGraphDraws_ = {};
    closeAttractorPreviewWindow();
}

void VulkanApp::requestGridResize(const GridSize newSize) {
    validateGridSize(newSize);

    const GridResizeResult resizeResult = controller_.resizeGrid(newSize, std::chrono::steady_clock::now());
    closeAttractorPreviewWindow();

    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }

    resetView();

    if (resizeResult.dimensionsChanged) {
        for (BufferAllocation& stagingBuffer : stagingBuffers_) {
            destroyBuffer(stagingBuffer);
        }
        destroySimulationBuffers();
        createSimulationBuffers();
        createStagingBuffers();
        updateDescriptorSets();
        currentSimulationBufferIndex_ = 0;
        pendingGpuSimulationStep_ = false;
        gpuStepSubmitted_ = false;
        gpuPhysarumLastCells_ = 0;
        gpuMinimumPhysarumCells_ = 0;
        gpuMinimumCheck_ = 0;
    }

    logGridConfiguration();
    refreshWindowTitle();
}

void VulkanApp::refreshWindowTitle() const {
    if (window_ == nullptr) {
        return;
    }

    const std::string title = viewModel_.windowTitle(model_, zoom_);
    glfwSetWindowTitle(window_, title.c_str());
}

void VulkanApp::logGridConfiguration() const {
    const GridSize size = requestedGridSize_;
    const double stateBufferMiB =
        static_cast<double>(size.w) * static_cast<double>(size.h) * 4.0 / (1024.0 * 1024.0);
    const double gpuStateTotalMiB = stateBufferMiB * 2.0;
    const double stagingTotalMiB = stateBufferMiB * static_cast<double>(kMaxFramesInFlight);
    const double simulationCpuMiB =
        static_cast<double>(size.w) * static_cast<double>(size.h) * (2.0 * 3.0 + 4.0) / (1024.0 * 1024.0);

    std::cout << std::fixed << std::setprecision(2)
              << "Grid size: " << size.w << 'x' << size.h << '\n'
              << "Estimated GPU state buffers total: " << gpuStateTotalMiB << " MiB\n"
              << "Estimated persistent upload staging total: " << stagingTotalMiB << " MiB\n"
              << "Estimated CPU sim memory (3 matrices + RGBA cache): " << simulationCpuMiB << " MiB\n"
              << "Simulation backend: " << (simulationGpuEnabled_ ? "SIM GPU" : "SIM CPU") << '\n'
              << "Upload policy: partial upload when dirty <= "
              << (kPartialUploadThreshold * 100.0)
              << "% of the grid, otherwise full upload.\n";
}

void VulkanApp::validateGridSize(const GridSize size) const {
    if (size.w == 0 || size.h == 0) {
        throw std::runtime_error("Grid size must be greater than zero.");
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        return;
    }

    const VkDeviceSize requiredStorageRange =
        static_cast<VkDeviceSize>(size.w) * static_cast<VkDeviceSize>(size.h) * sizeof(uint32_t);
    if (requiredStorageRange > physicalDeviceProperties_.limits.maxStorageBufferRange) {
        throw std::runtime_error(
            "Requested grid " + std::to_string(size.w) + "x" + std::to_string(size.h) +
            " exceeds device maxStorageBufferRange=" +
            std::to_string(physicalDeviceProperties_.limits.maxStorageBufferRange));
    }
}

void VulkanApp::resetAttractorPreviewBounds() {
    attractorPreviewBoundsInitialized_ = false;
    attractorPreviewRenderCapped_ = false;
    attractorPreviewWorldMinX_ = 0.0f;
    attractorPreviewWorldMinY_ = 0.0f;
    attractorPreviewWorldMaxX_ = 0.0f;
    attractorPreviewWorldMaxY_ = 0.0f;
}

void VulkanApp::openAttractorPreviewWindow() {
    if (attractorWindow_.window != nullptr) {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    attractorWindow_.window = glfwCreateWindow(
        kAttractorPreviewWidth,
        kAttractorPreviewHeight,
        kAttractorPreviewWindowName,
        nullptr,
        nullptr);
    if (attractorWindow_.window == nullptr) {
        throw std::runtime_error("Failed to create attractor preview window.");
    }

    glfwSetWindowUserPointer(attractorWindow_.window, this);
    glfwSetFramebufferSizeCallback(attractorWindow_.window, attractorFramebufferResizeCallback);

    try {
        createAttractorPreviewSurface();
        createAttractorPreviewSwapChain();
        createAttractorPreviewImageViews();
        createAttractorPreviewRenderPass();
        createAttractorPreviewPipeline();
        createAttractorPreviewFramebuffers();
        createAttractorPreviewCommandResources();
        createAttractorPreviewSyncObjects();
    } catch (...) {
        closeAttractorPreviewWindow();
        throw;
    }
}

void VulkanApp::closeAttractorPreviewWindow() {
    if (device_ != VK_NULL_HANDLE && attractorWindow_.window != nullptr) {
        vkDeviceWaitIdle(device_);
    }

    cleanupAttractorPreviewSwapChain();

    if (attractorWindow_.inFlightFence != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, attractorWindow_.inFlightFence, nullptr);
        attractorWindow_.inFlightFence = VK_NULL_HANDLE;
    }
    if (attractorWindow_.imageAvailableSemaphore != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, attractorWindow_.imageAvailableSemaphore, nullptr);
        attractorWindow_.imageAvailableSemaphore = VK_NULL_HANDLE;
    }
    if (attractorWindow_.renderFinishedSemaphore != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, attractorWindow_.renderFinishedSemaphore, nullptr);
        attractorWindow_.renderFinishedSemaphore = VK_NULL_HANDLE;
    }
    if (attractorWindow_.commandPool != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, attractorWindow_.commandPool, nullptr);
        attractorWindow_.commandPool = VK_NULL_HANDLE;
    }
    attractorWindow_.commandBuffer = VK_NULL_HANDLE;

    if (attractorWindow_.surface != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, attractorWindow_.surface, nullptr);
        attractorWindow_.surface = VK_NULL_HANDLE;
    }
    if (attractorWindow_.window != nullptr) {
        glfwDestroyWindow(attractorWindow_.window);
        attractorWindow_.window = nullptr;
    }

    attractorWindow_.presentFamilyIndex = 0;
    attractorWindow_.presentQueue = VK_NULL_HANDLE;
    attractorWindow_.framebufferResized = false;
    controller_.notifyAttractorPreviewClosed();
    resetAttractorPreviewBounds();
}

void VulkanApp::updateAttractorPreviewWindow() {
    if (attractorWindow_.window == nullptr) {
        return;
    }

    if (glfwWindowShouldClose(attractorWindow_.window) == GLFW_TRUE) {
        closeAttractorPreviewWindow();
        return;
    }

    try {
        drawAttractorPreviewFrame();
    } catch (const std::exception&) {
        attractorGraphVertexCount_ = 0;
        attractorGraphDraws_ = {};
        closeAttractorPreviewWindow();
        controller_.setAttractorError();
    }
}

void VulkanApp::renderAttractorStatusPreview(const std::string& statusText) {
    openAttractorPreviewWindow();
    glfwSetWindowTitle(
        attractorWindow_.window,
        (std::string(kAttractorPreviewWindowName) + " | " + statusText).c_str());
}

void VulkanApp::renderAttractorPreview(const AttractorGraph& graph) {
    constexpr float kWorldPadding = 28.0f;
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 500.0f;
    float maxY = 500.0f;
    if (!graph.nodes.empty()) {
        minX = graph.nodes.front().x;
        minY = graph.nodes.front().y;
        maxX = graph.nodes.front().x;
        maxY = graph.nodes.front().y;
        for (const AttractorGraphNode& node : graph.nodes) {
            minX = std::min(minX, node.x);
            minY = std::min(minY, node.y);
            maxX = std::max(maxX, node.x);
            maxY = std::max(maxY, node.y);
        }
    }

    minX -= kWorldPadding;
    minY -= kWorldPadding;
    maxX += kWorldPadding;
    maxY += kWorldPadding;

    if (!attractorPreviewBoundsInitialized_) {
        attractorPreviewWorldMinX_ = minX;
        attractorPreviewWorldMinY_ = minY;
        attractorPreviewWorldMaxX_ = maxX;
        attractorPreviewWorldMaxY_ = maxY;
        attractorPreviewBoundsInitialized_ = true;
    } else {
        attractorPreviewWorldMinX_ = std::min(attractorPreviewWorldMinX_, minX);
        attractorPreviewWorldMinY_ = std::min(attractorPreviewWorldMinY_, minY);
        attractorPreviewWorldMaxX_ = std::max(attractorPreviewWorldMaxX_, maxX);
        attractorPreviewWorldMaxY_ = std::max(attractorPreviewWorldMaxY_, maxY);
    }

    openAttractorPreviewWindow();
    rebuildAttractorGraphBuffer(graph);

    std::ostringstream details;
    details << graph.settings.width << 'x' << graph.settings.height
            << " | " << (graph.approximate ? "MODO APROX" : "MODO EXACTO")
            << " | PROC " << graph.processedSeeds
            << " | NOD " << graph.nodes.size()
            << " | EDGE " << graph.edges.size();
    if (attractorPreviewRenderCapped_) {
        details << " | VISTA PARCIAL";
    }

    glfwSetWindowTitle(
        attractorWindow_.window,
        (std::string(kAttractorPreviewWindowName) + " | " + details.str()).c_str());
}

void VulkanApp::createInstance() {
    if (kEnableValidationLayers && !checkValidationLayerSupport()) {
        std::cout << "Validation layer VK_LAYER_KHRONOS_validation not found; continuing without validation layers.\n";
        validationLayersEnabled_ = false;
    } else {
        validationLayersEnabled_ = kEnableValidationLayers;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "PhysarumVulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "None";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const std::vector<const char*> extensions = getRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (validationLayersEnabled_) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    throwIfFailed(vkCreateInstance(&createInfo, nullptr, &instance_), "Failed to create Vulkan instance");
}

void VulkanApp::setupDebugMessenger() {
    if (!validationLayersEnabled_) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    throwIfFailed(
        createDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_),
        "Failed to create debug messenger");
}

void VulkanApp::createSurface() {
    throwIfFailed(
        glfwCreateWindowSurface(instance_, window_, nullptr, &surface_),
        "Failed to create window surface");
}

void VulkanApp::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    throwIfFailed(vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr), "Failed to enumerate physical devices");
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan-compatible GPU found.");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    throwIfFailed(
        vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data()),
        "Failed to enumerate physical devices");

    int bestScore = std::numeric_limits<int>::min();
    for (const VkPhysicalDevice candidateDevice : devices) {
        VkPhysicalDeviceProperties candidateProperties{};
        vkGetPhysicalDeviceProperties(candidateDevice, &candidateProperties);

        std::cout << "Available GPU: " << candidateProperties.deviceName
                  << " (" << deviceTypeToString(candidateProperties.deviceType) << ")\n";

        const QueueFamilyIndices candidateIndices = findQueueFamilies(candidateDevice);
        if (!candidateIndices.isComplete() || !checkDeviceExtensionSupport(candidateDevice)) {
            continue;
        }

        const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(candidateDevice);
        if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
            continue;
        }

        int score = 0;
        switch (candidateProperties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score = 200;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score = 100;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score = 50;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                score = -100;
                break;
            default:
                score = 0;
                break;
        }

        if (score > bestScore) {
            bestScore = score;
            physicalDevice_ = candidateDevice;
            physicalDeviceProperties_ = candidateProperties;
            queueFamilyIndices_ = candidateIndices;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("No compatible GPU supports graphics, present and VK_KHR_swapchain.");
    }

    VkPhysicalDeviceFeatures selectedFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice_, &selectedFeatures);
    shaderInt64Supported_ = selectedFeatures.shaderInt64 == VK_TRUE;
    selectedGpuName_ = physicalDeviceProperties_.deviceName;
    selectedGpuType_ = deviceTypeToString(physicalDeviceProperties_.deviceType);
    std::cout << "Selected GPU: " << selectedGpuName_ << " (" << selectedGpuType_ << ")\n";
}

void VulkanApp::createLogicalDevice() {
    std::set<uint32_t> uniqueQueueFamilies = {
        queueFamilyIndices_.graphicsFamily.value(),
        queueFamilyIndices_.presentFamily.value()
    };
    if (queueFamilyIndices_.computeFamily.has_value()) {
        uniqueQueueFamilies.insert(queueFamilyIndices_.computeFamily.value());
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueFamilies.size());

    const float queuePriority = 1.0f;
    for (const uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    if (shaderInt64Supported_) {
        deviceFeatures.shaderInt64 = VK_TRUE;
    }

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions_.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions_.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    if (validationLayersEnabled_) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers_.size());
        createInfo.ppEnabledLayerNames = validationLayers_.data();
    }

    throwIfFailed(vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_), "Failed to create logical device");

    vkGetDeviceQueue(device_, queueFamilyIndices_.graphicsFamily.value(), 0, &graphicsQueue_);
    if (queueFamilyIndices_.computeFamily.has_value()) {
        vkGetDeviceQueue(device_, queueFamilyIndices_.computeFamily.value(), 0, &computeQueue_);
    }
    vkGetDeviceQueue(device_, queueFamilyIndices_.presentFamily.value(), 0, &presentQueue_);
    if (computeQueue_ == VK_NULL_HANDLE) {
        computeQueue_ = graphicsQueue_;
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());
    simulationGraphicsQueueComputeCapable_ =
        queueFamilyIndices_.graphicsFamily.has_value() &&
        queueFamilyIndices_.graphicsFamily.value() < queueFamilies.size() &&
        (queueFamilies[queueFamilyIndices_.graphicsFamily.value()].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U;
}

void VulkanApp::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices_.graphicsFamily.value();

    throwIfFailed(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_), "Failed to create command pool");
}

void VulkanApp::createSwapChain() {
    const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice_);
    const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t queueFamilyIndices[] = {
        queueFamilyIndices_.graphicsFamily.value(),
        queueFamilyIndices_.presentFamily.value()
    };
    if (queueFamilyIndices_.graphicsFamily != queueFamilyIndices_.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    throwIfFailed(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_), "Failed to create swapchain");

    throwIfFailed(vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr), "Failed to query swapchain images");
    swapChainImages_.resize(imageCount);
    throwIfFailed(
        vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data()),
        "Failed to get swapchain images");

    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_ = extent;
    imagesInFlight_.assign(swapChainImages_.size(), VK_NULL_HANDLE);
}

void VulkanApp::createImageViews() {
    swapChainImageViews_.resize(swapChainImages_.size());

    for (std::size_t index = 0; index < swapChainImages_.size(); ++index) {
        swapChainImageViews_[index] = createImageView(swapChainImages_[index], swapChainImageFormat_);
    }
}

void VulkanApp::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    throwIfFailed(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_), "Failed to create render pass");
}

void VulkanApp::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding statesBinding{};
    statesBinding.binding = 0;
    statesBinding.descriptorCount = 1;
    statesBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    statesBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding paletteBinding{};
    paletteBinding.binding = 1;
    paletteBinding.descriptorCount = 1;
    paletteBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    paletteBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    const std::array<VkDescriptorSetLayoutBinding, 2> bindings{{statesBinding, paletteBinding}};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    throwIfFailed(
        vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_),
        "Failed to create descriptor set layout");
}

void VulkanApp::createComputeDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding inputBinding{};
    inputBinding.binding = 0;
    inputBinding.descriptorCount = 1;
    inputBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding outputBinding{};
    outputBinding.binding = 1;
    outputBinding.descriptorCount = 1;
    outputBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding statsBinding{};
    statsBinding.binding = 2;
    statsBinding.descriptorCount = 1;
    statsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    statsBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    const std::array<VkDescriptorSetLayoutBinding, 3> bindings{{inputBinding, outputBinding, statsBinding}};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    throwIfFailed(
        vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &computeDescriptorSetLayout_),
        "Failed to create simulation compute descriptor set layout");
}

void VulkanApp::createPipelineLayouts() {
    VkPipelineLayoutCreateInfo texturedLayoutInfo{};
    texturedLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    texturedLayoutInfo.setLayoutCount = 1;
    texturedLayoutInfo.pSetLayouts = &descriptorSetLayout_;
    VkPushConstantRange quadPushConstantRange{};
    quadPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    quadPushConstantRange.offset = 0;
    quadPushConstantRange.size = sizeof(QuadPushConstants);
    texturedLayoutInfo.pushConstantRangeCount = 1;
    texturedLayoutInfo.pPushConstantRanges = &quadPushConstantRange;

    throwIfFailed(
        vkCreatePipelineLayout(device_, &texturedLayoutInfo, nullptr, &texturedPipelineLayout_),
        "Failed to create textured pipeline layout");

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(RectPushConstants);

    VkPipelineLayoutCreateInfo solidLayoutInfo{};
    solidLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    solidLayoutInfo.pushConstantRangeCount = 1;
    solidLayoutInfo.pPushConstantRanges = &pushConstantRange;

    throwIfFailed(
        vkCreatePipelineLayout(device_, &solidLayoutInfo, nullptr, &solidPipelineLayout_),
        "Failed to create solid pipeline layout");
}

void VulkanApp::createGraphicsPipelines() {
    const auto quadVertexCode = readBinaryFile(shaderPath("quad.vert.spv"));
    const auto quadFragmentCode = readBinaryFile(shaderPath("quad.frag.spv"));
    const auto rectVertexCode = readBinaryFile(shaderPath("rect.vert.spv"));
    const auto rectFragmentCode = readBinaryFile(shaderPath("rect.frag.spv"));

    const VkShaderModule quadVertexModule = createShaderModule(quadVertexCode);
    const VkShaderModule quadFragmentModule = createShaderModule(quadFragmentCode);
    const VkShaderModule rectVertexModule = createShaderModule(rectVertexCode);
    const VkShaderModule rectFragmentModule = createShaderModule(rectFragmentCode);

    const auto createPipeline = [&](const VkShaderModule vertexModule,
                                    const VkShaderModule fragmentModule,
                                    const auto& bindingDescription,
                                    const auto& attributeDescriptions,
                                    const VkPipelineLayout pipelineLayout,
                                    VkPipeline& pipeline) {
        VkPipelineShaderStageCreateInfo vertexStage{};
        vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexStage.module = vertexModule;
        vertexStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentStage{};
        fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentStage.module = fragmentModule;
        fragmentStage.pName = "main";

        const VkPipelineShaderStageCreateInfo shaderStages[] = {vertexStage, fragmentStage};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent_.width);
        viewport.height = static_cast<float>(swapChainExtent_.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent_;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass_;
        pipelineInfo.subpass = 0;

        throwIfFailed(
            vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline),
            "Failed to create graphics pipeline");
    };

    const auto texturedBinding = TexturedVertex::bindingDescription();
    const auto texturedAttributes = TexturedVertex::attributeDescriptions();
    const auto solidBinding = SolidVertex::bindingDescription();
    const auto solidAttributes = SolidVertex::attributeDescriptions();

    try {
        createPipeline(
            quadVertexModule,
            quadFragmentModule,
            texturedBinding,
            texturedAttributes,
            texturedPipelineLayout_,
            texturedPipeline_);
        createPipeline(
            rectVertexModule,
            rectFragmentModule,
            solidBinding,
            solidAttributes,
            solidPipelineLayout_,
            solidPipeline_);
    } catch (...) {
        vkDestroyShaderModule(device_, rectFragmentModule, nullptr);
        vkDestroyShaderModule(device_, rectVertexModule, nullptr);
        vkDestroyShaderModule(device_, quadFragmentModule, nullptr);
        vkDestroyShaderModule(device_, quadVertexModule, nullptr);
        throw;
    }

    vkDestroyShaderModule(device_, rectFragmentModule, nullptr);
    vkDestroyShaderModule(device_, rectVertexModule, nullptr);
    vkDestroyShaderModule(device_, quadFragmentModule, nullptr);
    vkDestroyShaderModule(device_, quadVertexModule, nullptr);
}

void VulkanApp::createComputePipeline() {
    if (!simulationGraphicsQueueComputeCapable_) {
        return;
    }

    const auto shaderCode = readBinaryFile(shaderPath("physarum.comp.spv"));
    const VkShaderModule shaderModule = createShaderModule(shaderCode);

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(uint32_t) * 4U;

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &computeDescriptorSetLayout_;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    throwIfFailed(
        vkCreatePipelineLayout(device_, &layoutInfo, nullptr, &computePipelineLayout_),
        "Failed to create simulation compute pipeline layout");

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = computePipelineLayout_;

    throwIfFailed(
        vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline_),
        "Failed to create simulation compute pipeline");

    vkDestroyShaderModule(device_, shaderModule, nullptr);
}

void VulkanApp::createFramebuffers() {
    swapChainFramebuffers_.resize(swapChainImageViews_.size());

    for (std::size_t index = 0; index < swapChainImageViews_.size(); ++index) {
        VkImageView attachments[] = {swapChainImageViews_[index]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent_.width;
        framebufferInfo.height = swapChainExtent_.height;
        framebufferInfo.layers = 1;

        throwIfFailed(
            vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapChainFramebuffers_[index]),
            "Failed to create framebuffer");
    }
}

void VulkanApp::createVertexBuffers() {
    constexpr VkDeviceSize kSolidUiBufferSize = static_cast<VkDeviceSize>(8192U) * sizeof(SolidVertex);

    const auto uploadStaticBuffer = [this](const void* sourceData,
                                           const VkDeviceSize bufferSize,
                                           const VkBufferUsageFlags usage,
                                           BufferAllocation& target) {
        BufferAllocation staging{};
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            staging);

        void* mapped = nullptr;
        throwIfFailed(vkMapMemory(device_, staging.memory, 0, bufferSize, 0, &mapped), "Failed to map staging buffer");
        std::memcpy(mapped, sourceData, static_cast<std::size_t>(bufferSize));
        vkUnmapMemory(device_, staging.memory);

        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            target);

        copyBuffer(staging, target, bufferSize);
        destroyBuffer(staging);
    };

    uploadStaticBuffer(
        kSimulationQuadVertices.data(),
        static_cast<VkDeviceSize>(sizeof(kSimulationQuadVertices)),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        texturedVertexBuffer_);

    destroyBuffer(solidVertexBuffer_);
    createBuffer(
        kSolidUiBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        solidVertexBuffer_);
    throwIfFailed(
        vkMapMemory(device_, solidVertexBuffer_.memory, 0, kSolidUiBufferSize, 0, &solidVertexBuffer_.mapped),
        "Failed to map solid UI buffer");
    rebuildSolidUiBuffer();
}

void VulkanApp::createOverlayTextBuffer() {
    const VkDeviceSize bufferSize =
        static_cast<VkDeviceSize>(kOverlayTextVertexCapacity) * sizeof(SolidVertex);

    destroyBuffer(overlayTextVertexBuffer_);
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        overlayTextVertexBuffer_);
    throwIfFailed(
        vkMapMemory(device_, overlayTextVertexBuffer_.memory, 0, bufferSize, 0, &overlayTextVertexBuffer_.mapped),
        "Failed to map overlay text buffer");

    overlayTextVertexCount_ = 0;
    updateOverlayTextBuffer();
}

void VulkanApp::createAttractorGraphBuffer() {
    const VkDeviceSize bufferSize =
        static_cast<VkDeviceSize>(kAttractorGraphVertexCapacity) * sizeof(SolidVertex);

    destroyBuffer(attractorGraphVertexBuffer_);
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        attractorGraphVertexBuffer_);
    throwIfFailed(
        vkMapMemory(device_, attractorGraphVertexBuffer_.memory, 0, bufferSize, 0, &attractorGraphVertexBuffer_.mapped),
        "Failed to map attractor graph buffer");

    attractorGraphVertexCount_ = 0;
    attractorGraphDraws_ = {};
    attractorPreviewRenderCapped_ = false;
}

void VulkanApp::rebuildAttractorGraphBuffer(const AttractorGraph& graph) {
    if (attractorGraphVertexBuffer_.mapped == nullptr) {
        attractorGraphVertexCount_ = 0;
        attractorGraphDraws_ = {};
        attractorPreviewRenderCapped_ = false;
        return;
    }

    const float previewWidth =
        attractorWindow_.swapChainExtent.width > 0
            ? static_cast<float>(attractorWindow_.swapChainExtent.width)
            : static_cast<float>(kAttractorPreviewWidth);
    const float previewHeight =
        attractorWindow_.swapChainExtent.height > 0
            ? static_cast<float>(attractorWindow_.swapChainExtent.height)
            : static_cast<float>(kAttractorPreviewHeight);
    constexpr float kMarginLeft = 56.0f;
    constexpr float kMarginTop = 56.0f;
    constexpr float kMarginRight = 56.0f;
    constexpr float kMarginBottom = 56.0f;

    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 500.0f;
    float maxY = 500.0f;
    if (!graph.nodes.empty()) {
        minX = graph.nodes.front().x;
        minY = graph.nodes.front().y;
        maxX = graph.nodes.front().x;
        maxY = graph.nodes.front().y;
        for (const AttractorGraphNode& node : graph.nodes) {
            minX = std::min(minX, node.x);
            minY = std::min(minY, node.y);
            maxX = std::max(maxX, node.x);
            maxY = std::max(maxY, node.y);
        }
    }
    if (!attractorPreviewBoundsInitialized_) {
        attractorPreviewWorldMinX_ = minX - 28.0f;
        attractorPreviewWorldMinY_ = minY - 28.0f;
        attractorPreviewWorldMaxX_ = maxX + 28.0f;
        attractorPreviewWorldMaxY_ = maxY + 28.0f;
        attractorPreviewBoundsInitialized_ = true;
    }

    const float worldWidth = std::max(180.0f, attractorPreviewWorldMaxX_ - attractorPreviewWorldMinX_);
    const float worldHeight = std::max(180.0f, attractorPreviewWorldMaxY_ - attractorPreviewWorldMinY_);
    const float contentWidth = std::max(1.0f, previewWidth - kMarginLeft - kMarginRight);
    const float contentHeight = std::max(1.0f, previewHeight - kMarginTop - kMarginBottom);
    const float scale = std::min(contentWidth / worldWidth, contentHeight / worldHeight);
    const float offsetX = kMarginLeft + (contentWidth - worldWidth * scale) * 0.5f;
    const float offsetY = kMarginTop + (contentHeight - worldHeight * scale) * 0.5f;

    const auto pixelToPreviewNdcX = [previewWidth](const float x) {
        return (x / previewWidth) * 2.0f - 1.0f;
    };
    const auto pixelToPreviewNdcY = [previewHeight](const float y) {
        return (y / previewHeight) * 2.0f - 1.0f;
    };
    const auto toPreviewPoint = [&](const float x, const float y) {
        return std::pair<float, float>{
            offsetX + (x - attractorPreviewWorldMinX_) * scale,
            offsetY + (y - attractorPreviewWorldMinY_) * scale
        };
    };
    const auto appendPreviewRect = [&](std::vector<SolidVertex>& vertices,
                                       const float rectMinX,
                                       const float rectMinY,
                                       const float rectMaxX,
                                       const float rectMaxY) {
        const uint32_t firstVertex = static_cast<uint32_t>(vertices.size());
        vertices.push_back({{pixelToPreviewNdcX(rectMinX), pixelToPreviewNdcY(rectMinY)}});
        vertices.push_back({{pixelToPreviewNdcX(rectMaxX), pixelToPreviewNdcY(rectMinY)}});
        vertices.push_back({{pixelToPreviewNdcX(rectMaxX), pixelToPreviewNdcY(rectMaxY)}});
        vertices.push_back({{pixelToPreviewNdcX(rectMinX), pixelToPreviewNdcY(rectMinY)}});
        vertices.push_back({{pixelToPreviewNdcX(rectMaxX), pixelToPreviewNdcY(rectMaxY)}});
        vertices.push_back({{pixelToPreviewNdcX(rectMinX), pixelToPreviewNdcY(rectMaxY)}});
        return SolidDrawRange{
            firstVertex,
            static_cast<uint32_t>(vertices.size()) - firstVertex
        };
    };
    const auto appendPreviewGlyph = [&](std::vector<SolidVertex>& vertices,
                                        const char glyph,
                                        const float x,
                                        const float y,
                                        const float pixelSize) {
        const auto rows = glyphPattern(glyph);
        for (uint32_t row = 0; row < rows.size(); ++row) {
            for (uint32_t column = 0; column < 5; ++column) {
                const uint8_t mask = static_cast<uint8_t>(1U << (4U - column));
                if ((rows[row] & mask) == 0U) {
                    continue;
                }

                const float minGlyphX = x + static_cast<float>(column) * pixelSize;
                const float minGlyphY = y + static_cast<float>(row) * pixelSize;
                appendPreviewRect(
                    vertices,
                    minGlyphX,
                    minGlyphY,
                    minGlyphX + pixelSize,
                    minGlyphY + pixelSize);
            }
        }
    };
    const auto appendPreviewText = [&](std::vector<SolidVertex>& vertices,
                                       const std::string_view text,
                                       const float startX,
                                       const float startY,
                                       const float pixelSize) {
        const uint32_t firstVertex = static_cast<uint32_t>(vertices.size());
        const float glyphAdvance = 6.0f * pixelSize;
        const float spaceAdvance = 4.0f * pixelSize;
        float cursorX = startX;
        for (const char glyph : text) {
            appendPreviewGlyph(vertices, glyph, cursorX, startY, pixelSize);
            cursorX += glyph == ' ' ? spaceAdvance : glyphAdvance;
        }
        return SolidDrawRange{
            firstVertex,
            static_cast<uint32_t>(vertices.size()) - firstVertex
        };
    };
    const auto appendPreviewThickLine = [&](std::vector<SolidVertex>& vertices,
                                            const float startX,
                                            const float startY,
                                            const float endX,
                                            const float endY,
                                            const float thickness) {
        const float deltaX = endX - startX;
        const float deltaY = endY - startY;
        const float length = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (length <= 0.0001f) {
            return appendPreviewRect(
                vertices,
                startX - thickness * 0.5f,
                startY - thickness * 0.5f,
                startX + thickness * 0.5f,
                startY + thickness * 0.5f);
        }

        const float normalX = -deltaY / length * thickness * 0.5f;
        const float normalY = deltaX / length * thickness * 0.5f;
        const uint32_t firstVertex = static_cast<uint32_t>(vertices.size());
        vertices.push_back({{pixelToPreviewNdcX(startX + normalX), pixelToPreviewNdcY(startY + normalY)}});
        vertices.push_back({{pixelToPreviewNdcX(endX + normalX), pixelToPreviewNdcY(endY + normalY)}});
        vertices.push_back({{pixelToPreviewNdcX(endX - normalX), pixelToPreviewNdcY(endY - normalY)}});
        vertices.push_back({{pixelToPreviewNdcX(startX + normalX), pixelToPreviewNdcY(startY + normalY)}});
        vertices.push_back({{pixelToPreviewNdcX(endX - normalX), pixelToPreviewNdcY(endY - normalY)}});
        vertices.push_back({{pixelToPreviewNdcX(startX - normalX), pixelToPreviewNdcY(startY - normalY)}});
        return SolidDrawRange{
            firstVertex,
            static_cast<uint32_t>(vertices.size()) - firstVertex
        };
    };

    std::size_t cycleNodeCount = 0;
    std::size_t plainNodeCount = 0;
    for (const AttractorGraphNode& node : graph.nodes) {
        if (node.cycle) {
            ++cycleNodeCount;
        } else {
            ++plainNodeCount;
        }
    }

    constexpr std::size_t kVerticesPerPrimitive = 6;
    const std::size_t graphVertexBudget =
        kAttractorGraphVertexCapacity > kAttractorPreviewLegendVertexBudget
            ? static_cast<std::size_t>(kAttractorGraphVertexCapacity - kAttractorPreviewLegendVertexBudget)
            : 0U;
    const std::size_t maxRenderablePrimitives = graphVertexBudget / kVerticesPerPrimitive;
    const std::size_t edgePrimitiveCount = graph.edges.size();
    const std::size_t nodePrimitiveCount = plainNodeCount;
    const std::size_t cyclePrimitiveCount = cycleNodeCount;
    std::size_t remainingPrimitiveBudget = maxRenderablePrimitives;

    const std::size_t cycleBudget = std::min(cyclePrimitiveCount, remainingPrimitiveBudget);
    remainingPrimitiveBudget -= cycleBudget;

    std::size_t edgeBudget = 0;
    std::size_t nodeBudget = 0;
    const std::size_t regularPrimitiveCount = edgePrimitiveCount + nodePrimitiveCount;
    if (remainingPrimitiveBudget > 0 && regularPrimitiveCount > 0) {
        if (regularPrimitiveCount <= remainingPrimitiveBudget) {
            edgeBudget = edgePrimitiveCount;
            nodeBudget = nodePrimitiveCount;
        } else {
            nodeBudget = std::min(nodePrimitiveCount, std::max<std::size_t>(1U, remainingPrimitiveBudget / 10U));
            edgeBudget = std::min(edgePrimitiveCount, remainingPrimitiveBudget - nodeBudget);

            std::size_t assignedPrimitives = edgeBudget + nodeBudget;
            while (assignedPrimitives < remainingPrimitiveBudget) {
                bool assigned = false;
                if (edgeBudget < edgePrimitiveCount) {
                    ++edgeBudget;
                    ++assignedPrimitives;
                    assigned = true;
                } else if (nodeBudget < nodePrimitiveCount) {
                    ++nodeBudget;
                    ++assignedPrimitives;
                    assigned = true;
                }
                if (!assigned) {
                    break;
                }
            }
        }
    }

    attractorPreviewRenderCapped_ =
        edgeBudget < edgePrimitiveCount ||
        nodeBudget < nodePrimitiveCount ||
        cycleBudget < cyclePrimitiveCount;

    const float densityScale = std::clamp(
        120.0f / std::sqrt(static_cast<float>(graph.nodes.size() + graph.edges.size() + 1U)),
        0.30f,
        1.0f);
    const float edgeThickness = 0.8f + 1.4f * densityScale;
    const float nodeHalfSize = 1.25f + 3.25f * densityScale;
    const float cycleHalfSize = 2.0f + 5.0f * densityScale;

    std::vector<SolidVertex> vertices;
    vertices.reserve((edgeBudget + nodeBudget + cycleBudget) * kVerticesPerPrimitive + kAttractorPreviewLegendVertexBudget);

    attractorGraphDraws_.edges = {
        static_cast<uint32_t>(vertices.size()),
        0U
    };
    for (std::size_t edgeIndex = 0; edgeIndex < graph.edges.size(); ++edgeIndex) {
        if (!shouldRenderSampledElement(edgeIndex, graph.edges.size(), edgeBudget)) {
            continue;
        }
        const auto& [origin, destination] = graph.edges[edgeIndex];
        const AttractorGraphNode& from = graph.nodes[origin];
        const AttractorGraphNode& to = graph.nodes[destination];
        const auto [fromX, fromY] = toPreviewPoint(from.x, from.y);
        const auto [toX, toY] = toPreviewPoint(to.x, to.y);
        appendPreviewThickLine(vertices, fromX, fromY, toX, toY, edgeThickness);
    }
    attractorGraphDraws_.edges.vertexCount =
        static_cast<uint32_t>(vertices.size()) - attractorGraphDraws_.edges.firstVertex;

    attractorGraphDraws_.nodes = {
        static_cast<uint32_t>(vertices.size()),
        0U
    };
    std::size_t plainNodeIndex = 0;
    for (const AttractorGraphNode& node : graph.nodes) {
        if (node.cycle) {
            continue;
        }
        if (!shouldRenderSampledElement(plainNodeIndex, plainNodeCount, nodeBudget)) {
            ++plainNodeIndex;
            continue;
        }
        const auto [centerX, centerY] = toPreviewPoint(node.x, node.y);
        appendPreviewRect(vertices, centerX - nodeHalfSize, centerY - nodeHalfSize, centerX + nodeHalfSize, centerY + nodeHalfSize);
        ++plainNodeIndex;
    }
    attractorGraphDraws_.nodes.vertexCount =
        static_cast<uint32_t>(vertices.size()) - attractorGraphDraws_.nodes.firstVertex;

    attractorGraphDraws_.cycles = {
        static_cast<uint32_t>(vertices.size()),
        0U
    };
    std::size_t cycleNodeIndex = 0;
    for (const AttractorGraphNode& node : graph.nodes) {
        if (!node.cycle) {
            continue;
        }
        if (!shouldRenderSampledElement(cycleNodeIndex, cycleNodeCount, cycleBudget)) {
            ++cycleNodeIndex;
            continue;
        }
        const auto [centerX, centerY] = toPreviewPoint(node.x, node.y);
        appendPreviewRect(vertices, centerX - cycleHalfSize, centerY - cycleHalfSize, centerX + cycleHalfSize, centerY + cycleHalfSize);
        ++cycleNodeIndex;
    }
    attractorGraphDraws_.cycles.vertexCount =
        static_cast<uint32_t>(vertices.size()) - attractorGraphDraws_.cycles.firstVertex;

    const float legendMinX = 20.0f;
    const float legendMinY = 20.0f;
    const float legendWidth = 380.0f;
    const float legendHeight = 138.0f;
    const float legendSwatchSize = 18.0f;
    const float legendTextX = legendMinX + 56.0f;
    const float legendTitleY = legendMinY + 16.0f;
    const float legendRowStartY = legendMinY + 52.0f;
    const float legendRowGap = 28.0f;

    attractorGraphDraws_.legendPanel = {
        static_cast<uint32_t>(vertices.size()),
        0U
    };
    appendPreviewRect(vertices, legendMinX, legendMinY, legendMinX + legendWidth, legendMinY + legendHeight);
    attractorGraphDraws_.legendPanel.vertexCount =
        static_cast<uint32_t>(vertices.size()) - attractorGraphDraws_.legendPanel.firstVertex;

    attractorGraphDraws_.legendEdgeSwatch = {
        static_cast<uint32_t>(vertices.size()),
        0U
    };
    appendPreviewRect(
        vertices,
        legendMinX + 18.0f,
        legendRowStartY - 2.0f,
        legendMinX + 18.0f + legendSwatchSize,
        legendRowStartY - 2.0f + legendSwatchSize);
    attractorGraphDraws_.legendEdgeSwatch.vertexCount =
        static_cast<uint32_t>(vertices.size()) - attractorGraphDraws_.legendEdgeSwatch.firstVertex;

    attractorGraphDraws_.legendNodeSwatch = {
        static_cast<uint32_t>(vertices.size()),
        0U
    };
    appendPreviewRect(
        vertices,
        legendMinX + 18.0f,
        legendRowStartY + legendRowGap - 2.0f,
        legendMinX + 18.0f + legendSwatchSize,
        legendRowStartY + legendRowGap - 2.0f + legendSwatchSize);
    attractorGraphDraws_.legendNodeSwatch.vertexCount =
        static_cast<uint32_t>(vertices.size()) - attractorGraphDraws_.legendNodeSwatch.firstVertex;

    attractorGraphDraws_.legendCycleSwatch = {
        static_cast<uint32_t>(vertices.size()),
        0U
    };
    appendPreviewRect(
        vertices,
        legendMinX + 18.0f,
        legendRowStartY + legendRowGap * 2.0f - 2.0f,
        legendMinX + 18.0f + legendSwatchSize,
        legendRowStartY + legendRowGap * 2.0f - 2.0f + legendSwatchSize);
    attractorGraphDraws_.legendCycleSwatch.vertexCount =
        static_cast<uint32_t>(vertices.size()) - attractorGraphDraws_.legendCycleSwatch.firstVertex;

    attractorGraphDraws_.legendText = {
        static_cast<uint32_t>(vertices.size()),
        0U
    };
    appendPreviewText(vertices, "LEYENDA", legendMinX + 18.0f, legendTitleY, 2.8f);
    appendPreviewText(vertices, "CIAN TRANSICION", legendTextX, legendRowStartY, 2.1f);
    appendPreviewText(vertices, "MAGENTA NODO", legendTextX, legendRowStartY + legendRowGap, 2.1f);
    appendPreviewText(vertices, "DORADO CICLO", legendTextX, legendRowStartY + legendRowGap * 2.0f, 2.1f);
    attractorGraphDraws_.legendText.vertexCount =
        static_cast<uint32_t>(vertices.size()) - attractorGraphDraws_.legendText.firstVertex;

    if (vertices.size() > kAttractorGraphVertexCapacity) {
        throw std::runtime_error("Attractor preview vertex budget exceeded.");
    }

    if (!vertices.empty()) {
        std::memcpy(
            attractorGraphVertexBuffer_.mapped,
            vertices.data(),
            vertices.size() * sizeof(SolidVertex));
    }
    attractorGraphVertexCount_ = static_cast<uint32_t>(vertices.size());
}

void VulkanApp::createSimulationBuffers() {
    const GridSize size = simulation_.gridSize();
    validateGridSize(size);

    const VkDeviceSize stateBufferSize =
        static_cast<VkDeviceSize>(size.w) * static_cast<VkDeviceSize>(size.h) * sizeof(uint32_t);
    for (BufferAllocation& stateBuffer : simulationStateBuffers_) {
        destroyBuffer(stateBuffer);
        createBuffer(
            stateBufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            stateBuffer);
    }

    destroyBuffer(simulationPaletteBuffer_);
    createBuffer(
        sizeof(uint32_t) * simulation_.palette().size(),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        simulationPaletteBuffer_);
    throwIfFailed(
        vkMapMemory(device_, simulationPaletteBuffer_.memory, 0, simulationPaletteBuffer_.size, 0, &simulationPaletteBuffer_.mapped),
        "Failed to map simulation palette buffer");

    destroyBuffer(simulationStatsBuffer_);
    createBuffer(
        sizeof(uint32_t) * 3U,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        simulationStatsBuffer_);
    throwIfFailed(
        vkMapMemory(device_, simulationStatsBuffer_.memory, 0, simulationStatsBuffer_.size, 0, &simulationStatsBuffer_.mapped),
        "Failed to map simulation stats buffer");

    auto* paletteWords = static_cast<uint32_t*>(simulationPaletteBuffer_.mapped);
    for (std::size_t index = 0; index < simulation_.palette().size(); ++index) {
        paletteWords[index] = packRgba8(simulation_.palette()[index]);
    }
    simulationPaletteDirty_ = false;
}

void VulkanApp::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 10;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 4;

    throwIfFailed(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_), "Failed to create descriptor pool");
}

void VulkanApp::createDescriptorSets() {
    std::array<VkDescriptorSetLayout, 2> renderLayouts{descriptorSetLayout_, descriptorSetLayout_};
    VkDescriptorSetAllocateInfo renderAllocateInfo{};
    renderAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    renderAllocateInfo.descriptorPool = descriptorPool_;
    renderAllocateInfo.descriptorSetCount = static_cast<uint32_t>(renderLayouts.size());
    renderAllocateInfo.pSetLayouts = renderLayouts.data();

    throwIfFailed(
        vkAllocateDescriptorSets(device_, &renderAllocateInfo, descriptorSets_.data()),
        "Failed to allocate simulation render descriptor sets");

    if (computePipeline_ != VK_NULL_HANDLE) {
        std::array<VkDescriptorSetLayout, 2> computeLayouts{computeDescriptorSetLayout_, computeDescriptorSetLayout_};
        VkDescriptorSetAllocateInfo computeAllocateInfo{};
        computeAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        computeAllocateInfo.descriptorPool = descriptorPool_;
        computeAllocateInfo.descriptorSetCount = static_cast<uint32_t>(computeLayouts.size());
        computeAllocateInfo.pSetLayouts = computeLayouts.data();

        throwIfFailed(
            vkAllocateDescriptorSets(device_, &computeAllocateInfo, computeDescriptorSets_.data()),
            "Failed to allocate simulation compute descriptor sets");
    }

    updateDescriptorSets();
}

void VulkanApp::updateDescriptorSets() {
    if (simulationPaletteBuffer_.buffer == VK_NULL_HANDLE) {
        return;
    }

    auto* paletteWords = static_cast<uint32_t*>(simulationPaletteBuffer_.mapped);
    for (std::size_t index = 0; index < simulation_.palette().size(); ++index) {
        paletteWords[index] = packRgba8(simulation_.palette()[index]);
    }

    for (std::size_t index = 0; index < descriptorSets_.size(); ++index) {
        if (descriptorSets_[index] == VK_NULL_HANDLE) {
            continue;
        }

        VkDescriptorBufferInfo statesInfo{};
        statesInfo.buffer = simulationStateBuffers_[index].buffer;
        statesInfo.offset = 0;
        statesInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo paletteInfo{};
        paletteInfo.buffer = simulationPaletteBuffer_.buffer;
        paletteInfo.offset = 0;
        paletteInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 2> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSets_[index];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &statesInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSets_[index];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo = &paletteInfo;

        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    if (computePipeline_ == VK_NULL_HANDLE) {
        return;
    }

    for (std::size_t index = 0; index < computeDescriptorSets_.size(); ++index) {
        if (computeDescriptorSets_[index] == VK_NULL_HANDLE) {
            continue;
        }

        VkDescriptorBufferInfo inputInfo{};
        inputInfo.buffer = simulationStateBuffers_[index].buffer;
        inputInfo.offset = 0;
        inputInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo outputInfo{};
        outputInfo.buffer = simulationStateBuffers_[1U - index].buffer;
        outputInfo.offset = 0;
        outputInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo statsInfo{};
        statsInfo.buffer = simulationStatsBuffer_.buffer;
        statsInfo.offset = 0;
        statsInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 3> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = computeDescriptorSets_[index];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &inputInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = computeDescriptorSets_[index];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo = &outputInfo;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = computeDescriptorSets_[index];
        writes[2].dstBinding = 2;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].descriptorCount = 1;
        writes[2].pBufferInfo = &statsInfo;

        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void VulkanApp::createStagingBuffers() {
    const GridSize size = simulation_.gridSize();
    const VkDeviceSize stagingSize =
        static_cast<VkDeviceSize>(size.w) * static_cast<VkDeviceSize>(size.h) * sizeof(uint32_t);

    for (BufferAllocation& stagingBuffer : stagingBuffers_) {
        destroyBuffer(stagingBuffer);
        createBuffer(
            stagingSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer);
        throwIfFailed(
            vkMapMemory(device_, stagingBuffer.memory, 0, stagingSize, 0, &stagingBuffer.mapped),
            "Failed to map persistent staging buffer");
    }
}

void VulkanApp::createCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    throwIfFailed(
        vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()),
        "Failed to allocate command buffers");
}

void VulkanApp::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (std::size_t index = 0; index < kMaxFramesInFlight; ++index) {
        throwIfFailed(
            vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[index]),
            "Failed to create image-available semaphore");
        throwIfFailed(
            vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[index]),
            "Failed to create render-finished semaphore");
        throwIfFailed(
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[index]),
            "Failed to create in-flight fence");
    }
}

void VulkanApp::createAttractorPreviewSurface() {
    throwIfFailed(
        glfwCreateWindowSurface(instance_, attractorWindow_.window, nullptr, &attractorWindow_.surface),
        "Failed to create attractor preview surface");

    const std::array<uint32_t, 2> candidateFamilies{{
        queueFamilyIndices_.presentFamily.value(),
        queueFamilyIndices_.graphicsFamily.value()
    }};
    for (const uint32_t familyIndex : candidateFamilies) {
        VkBool32 presentSupport = VK_FALSE;
        throwIfFailed(
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, familyIndex, attractorWindow_.surface, &presentSupport),
            "Failed to query attractor preview present support");
        if (presentSupport == VK_TRUE) {
            attractorWindow_.presentFamilyIndex = familyIndex;
            attractorWindow_.presentQueue =
                familyIndex == queueFamilyIndices_.graphicsFamily.value() ? graphicsQueue_ : presentQueue_;
            return;
        }
    }

    throw std::runtime_error("Selected GPU queue families cannot present the attractor preview surface.");
}

void VulkanApp::createAttractorPreviewSwapChain() {
    const SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(physicalDevice_, attractorWindow_.surface);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
        throw std::runtime_error("Attractor preview surface does not support a Vulkan swapchain.");
    }
    const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D extent =
        chooseSwapExtent(swapChainSupport.capabilities, attractorWindow_.window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = attractorWindow_.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t queueFamilyIndices[] = {
        queueFamilyIndices_.graphicsFamily.value(),
        attractorWindow_.presentFamilyIndex
    };
    if (queueFamilyIndices_.graphicsFamily.value() != attractorWindow_.presentFamilyIndex) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    throwIfFailed(
        vkCreateSwapchainKHR(device_, &createInfo, nullptr, &attractorWindow_.swapChain),
        "Failed to create attractor preview swapchain");

    throwIfFailed(
        vkGetSwapchainImagesKHR(device_, attractorWindow_.swapChain, &imageCount, nullptr),
        "Failed to query attractor preview swapchain images");
    attractorWindow_.swapChainImages.resize(imageCount);
    throwIfFailed(
        vkGetSwapchainImagesKHR(
            device_,
            attractorWindow_.swapChain,
            &imageCount,
            attractorWindow_.swapChainImages.data()),
        "Failed to get attractor preview swapchain images");

    attractorWindow_.swapChainImageFormat = surfaceFormat.format;
    attractorWindow_.swapChainExtent = extent;
    attractorWindow_.framebufferResized = false;
}

void VulkanApp::createAttractorPreviewImageViews() {
    attractorWindow_.swapChainImageViews.resize(attractorWindow_.swapChainImages.size());
    for (std::size_t index = 0; index < attractorWindow_.swapChainImages.size(); ++index) {
        attractorWindow_.swapChainImageViews[index] =
            createImageView(attractorWindow_.swapChainImages[index], attractorWindow_.swapChainImageFormat);
    }
}

void VulkanApp::createAttractorPreviewRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = attractorWindow_.swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    throwIfFailed(
        vkCreateRenderPass(device_, &renderPassInfo, nullptr, &attractorWindow_.renderPass),
        "Failed to create attractor preview render pass");
}

void VulkanApp::createAttractorPreviewPipeline() {
    const auto rectVertexCode = readBinaryFile(shaderPath("rect.vert.spv"));
    const auto rectFragmentCode = readBinaryFile(shaderPath("rect.frag.spv"));
    const VkShaderModule rectVertexModule = createShaderModule(rectVertexCode);
    const VkShaderModule rectFragmentModule = createShaderModule(rectFragmentCode);

    VkPipelineShaderStageCreateInfo vertexStage{};
    vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStage.module = rectVertexModule;
    vertexStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStage{};
    fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStage.module = rectFragmentModule;
    fragmentStage.pName = "main";

    const VkPipelineShaderStageCreateInfo shaderStages[] = {vertexStage, fragmentStage};
    const auto solidBinding = SolidVertex::bindingDescription();
    const auto solidAttributes = SolidVertex::attributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &solidBinding;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(solidAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = solidAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(attractorWindow_.swapChainExtent.width);
    viewport.height = static_cast<float>(attractorWindow_.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = attractorWindow_.swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = solidPipelineLayout_;
    pipelineInfo.renderPass = attractorWindow_.renderPass;
    pipelineInfo.subpass = 0;

    const VkResult result = vkCreateGraphicsPipelines(
        device_,
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &attractorWindow_.solidPipeline);
    vkDestroyShaderModule(device_, rectFragmentModule, nullptr);
    vkDestroyShaderModule(device_, rectVertexModule, nullptr);
    throwIfFailed(result, "Failed to create attractor preview pipeline");
}

void VulkanApp::createAttractorPreviewFramebuffers() {
    attractorWindow_.framebuffers.resize(attractorWindow_.swapChainImageViews.size());
    for (std::size_t index = 0; index < attractorWindow_.swapChainImageViews.size(); ++index) {
        VkImageView attachments[] = {attractorWindow_.swapChainImageViews[index]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = attractorWindow_.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = attractorWindow_.swapChainExtent.width;
        framebufferInfo.height = attractorWindow_.swapChainExtent.height;
        framebufferInfo.layers = 1;

        throwIfFailed(
            vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &attractorWindow_.framebuffers[index]),
            "Failed to create attractor preview framebuffer");
    }
}

void VulkanApp::createAttractorPreviewCommandResources() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices_.graphicsFamily.value();
    throwIfFailed(
        vkCreateCommandPool(device_, &poolInfo, nullptr, &attractorWindow_.commandPool),
        "Failed to create attractor preview command pool");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = attractorWindow_.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    throwIfFailed(
        vkAllocateCommandBuffers(device_, &allocInfo, &attractorWindow_.commandBuffer),
        "Failed to allocate attractor preview command buffer");
}

void VulkanApp::createAttractorPreviewSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    throwIfFailed(
        vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &attractorWindow_.imageAvailableSemaphore),
        "Failed to create attractor preview image-available semaphore");
    throwIfFailed(
        vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &attractorWindow_.renderFinishedSemaphore),
        "Failed to create attractor preview render-finished semaphore");
    throwIfFailed(
        vkCreateFence(device_, &fenceInfo, nullptr, &attractorWindow_.inFlightFence),
        "Failed to create attractor preview in-flight fence");
}

void VulkanApp::recreateAttractorPreviewSwapChain() {
    if (attractorWindow_.window == nullptr) {
        return;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(attractorWindow_.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwWaitEvents();
        if (attractorWindow_.window == nullptr) {
            return;
        }
        glfwGetFramebufferSize(attractorWindow_.window, &width, &height);
    }

    vkDeviceWaitIdle(device_);

    cleanupAttractorPreviewSwapChain();
    createAttractorPreviewSwapChain();
    createAttractorPreviewImageViews();
    createAttractorPreviewRenderPass();
    createAttractorPreviewPipeline();
    createAttractorPreviewFramebuffers();
    if (latestAttractorGraph_.has_value()) {
        rebuildAttractorGraphBuffer(latestAttractorGraph_.value());
    }
}

void VulkanApp::cleanupAttractorPreviewSwapChain() {
    for (VkFramebuffer framebuffer : attractorWindow_.framebuffers) {
        if (framebuffer != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
    }
    attractorWindow_.framebuffers.clear();

    if (attractorWindow_.solidPipeline != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, attractorWindow_.solidPipeline, nullptr);
        attractorWindow_.solidPipeline = VK_NULL_HANDLE;
    }
    if (attractorWindow_.renderPass != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, attractorWindow_.renderPass, nullptr);
        attractorWindow_.renderPass = VK_NULL_HANDLE;
    }

    for (VkImageView imageView : attractorWindow_.swapChainImageViews) {
        if (imageView != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, imageView, nullptr);
        }
    }
    attractorWindow_.swapChainImageViews.clear();

    if (attractorWindow_.swapChain != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, attractorWindow_.swapChain, nullptr);
        attractorWindow_.swapChain = VK_NULL_HANDLE;
    }

    attractorWindow_.swapChainImages.clear();
    attractorWindow_.swapChainImageFormat = VK_FORMAT_UNDEFINED;
    attractorWindow_.swapChainExtent = {};
}

void VulkanApp::recordAttractorPreviewCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    throwIfFailed(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin attractor preview command buffer");

    VkClearValue clearColor{};
    clearColor.color = {{
        kAttractorPreviewClearColor[0],
        kAttractorPreviewClearColor[1],
        kAttractorPreviewClearColor[2],
        kAttractorPreviewClearColor[3]
    }};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = attractorWindow_.renderPass;
    renderPassInfo.framebuffer = attractorWindow_.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = attractorWindow_.swapChainExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (attractorGraphVertexCount_ > 0U) {
        const VkDeviceSize zeroOffset = 0;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, attractorWindow_.solidPipeline);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &attractorGraphVertexBuffer_.buffer, &zeroOffset);

        const auto drawRange = [&](const SolidDrawRange& range, const std::array<float, 4>& color) {
            RectPushConstants push{};
            std::memcpy(push.color, color.data(), sizeof(push.color));
            vkCmdPushConstants(
                commandBuffer,
                solidPipelineLayout_,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(RectPushConstants),
                &push);
            vkCmdDraw(commandBuffer, range.vertexCount, 1, range.firstVertex, 0);
        };

        if (attractorGraphDraws_.edges.vertexCount > 0U) {
            drawRange(attractorGraphDraws_.edges, kAttractorPreviewEdgeColor);
        }
        if (attractorGraphDraws_.nodes.vertexCount > 0U) {
            drawRange(attractorGraphDraws_.nodes, kAttractorPreviewNodeColor);
        }
        if (attractorGraphDraws_.cycles.vertexCount > 0U) {
            drawRange(attractorGraphDraws_.cycles, kAttractorPreviewCycleColor);
        }
        if (attractorGraphDraws_.legendPanel.vertexCount > 0U) {
            drawRange(attractorGraphDraws_.legendPanel, kAttractorPreviewPanelColor);
        }
        if (attractorGraphDraws_.legendEdgeSwatch.vertexCount > 0U) {
            drawRange(attractorGraphDraws_.legendEdgeSwatch, kAttractorPreviewEdgeColor);
        }
        if (attractorGraphDraws_.legendNodeSwatch.vertexCount > 0U) {
            drawRange(attractorGraphDraws_.legendNodeSwatch, kAttractorPreviewNodeColor);
        }
        if (attractorGraphDraws_.legendCycleSwatch.vertexCount > 0U) {
            drawRange(attractorGraphDraws_.legendCycleSwatch, kAttractorPreviewCycleColor);
        }
        if (attractorGraphDraws_.legendText.vertexCount > 0U) {
            drawRange(attractorGraphDraws_.legendText, kAttractorPreviewTextColor);
        }
    }

    vkCmdEndRenderPass(commandBuffer);
    throwIfFailed(vkEndCommandBuffer(commandBuffer), "Failed to record attractor preview command buffer");
}

void VulkanApp::drawAttractorPreviewFrame() {
    if (attractorWindow_.window == nullptr || attractorWindow_.swapChain == VK_NULL_HANDLE) {
        return;
    }

    throwIfFailed(
        vkWaitForFences(device_, 1, &attractorWindow_.inFlightFence, VK_TRUE, UINT64_MAX),
        "Failed to wait for attractor preview fence");

    uint32_t imageIndex = 0;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        device_,
        attractorWindow_.swapChain,
        UINT64_MAX,
        attractorWindow_.imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &imageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateAttractorPreviewSwapChain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire attractor preview swapchain image.");
    }

    throwIfFailed(vkResetFences(device_, 1, &attractorWindow_.inFlightFence), "Failed to reset attractor preview fence");
    throwIfFailed(
        vkResetCommandBuffer(attractorWindow_.commandBuffer, 0),
        "Failed to reset attractor preview command buffer");
    recordAttractorPreviewCommandBuffer(attractorWindow_.commandBuffer, imageIndex);

    VkSemaphore waitSemaphores[] = {attractorWindow_.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {attractorWindow_.renderFinishedSemaphore};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &attractorWindow_.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    {
        std::lock_guard<std::mutex> lock(queueSubmitMutex_);
        throwIfFailed(
            vkQueueSubmit(graphicsQueue_, 1, &submitInfo, attractorWindow_.inFlightFence),
            "Failed to submit attractor preview command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &attractorWindow_.swapChain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = VK_SUCCESS;
    {
        std::lock_guard<std::mutex> lock(queueSubmitMutex_);
        presentResult = vkQueuePresentKHR(attractorWindow_.presentQueue, &presentInfo);
    }
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        presentResult == VK_SUBOPTIMAL_KHR ||
        attractorWindow_.framebufferResized) {
        attractorWindow_.framebufferResized = false;
        recreateAttractorPreviewSwapChain();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present attractor preview swapchain image.");
    }
}

void VulkanApp::recreateSwapChain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);

    while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window_, &width, &height);
    }

    vkDeviceWaitIdle(device_);

    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipelines();
    createFramebuffers();
}

void VulkanApp::cleanupSwapChain() {
    for (VkFramebuffer framebuffer : swapChainFramebuffers_) {
        if (framebuffer != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
    }
    swapChainFramebuffers_.clear();

    if (solidPipeline_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, solidPipeline_, nullptr);
        solidPipeline_ = VK_NULL_HANDLE;
    }
    if (texturedPipeline_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, texturedPipeline_, nullptr);
        texturedPipeline_ = VK_NULL_HANDLE;
    }
    if (renderPass_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }

    for (VkImageView imageView : swapChainImageViews_) {
        if (imageView != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, imageView, nullptr);
        }
    }
    swapChainImageViews_.clear();

    if (swapChain_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device_, swapChain_, nullptr);
        swapChain_ = VK_NULL_HANDLE;
    }

    swapChainImages_.clear();
    imagesInFlight_.clear();
}

void VulkanApp::destroySimulationBuffers() {
    for (BufferAllocation& stateBuffer : simulationStateBuffers_) {
        destroyBuffer(stateBuffer);
    }
    destroyBuffer(simulationPaletteBuffer_);
    destroyBuffer(simulationStatsBuffer_);
}

void VulkanApp::destroyBuffer(BufferAllocation& allocation) {
    if (allocation.mapped != nullptr && device_ != VK_NULL_HANDLE && allocation.memory != VK_NULL_HANDLE) {
        vkUnmapMemory(device_, allocation.memory);
        allocation.mapped = nullptr;
    }
    if (allocation.buffer != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, allocation.buffer, nullptr);
        allocation.buffer = VK_NULL_HANDLE;
    }
    if (allocation.memory != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, allocation.memory, nullptr);
        allocation.memory = VK_NULL_HANDLE;
    }
    allocation.size = 0;
}

void VulkanApp::rebuildSolidUiBuffer() {
    if (solidVertexBuffer_.mapped == nullptr) {
        return;
    }

    const SolidGeometry solidGeometry = buildSolidGeometry(sidebarScrollOffset_);
    if (static_cast<VkDeviceSize>(solidGeometry.vertices.size() * sizeof(SolidVertex)) > solidVertexBuffer_.size) {
        throw std::runtime_error("Solid UI buffer capacity exceeded.");
    }

    const auto assignUiElement = [](UiElement& target, const UiGeometryElement& source) {
        target.rect = ScreenRect{
            source.rect.minX,
            source.rect.minY,
            std::max(0.0f, source.rect.maxX - source.rect.minX),
            std::max(0.0f, source.rect.maxY - source.rect.minY)
        };
        target.draw = source.draw;
    };

    panelDraw_ = solidGeometry.panel;
    sidebarDraw_ = solidGeometry.sidebar;
    indicatorDraw_ = solidGeometry.indicator;
    scrollbarTrackDraw_ = solidGeometry.scrollbarTrack;
    scrollbarThumbDraw_ = solidGeometry.scrollbarThumb;
    assignUiElement(loadMapButton_, solidGeometry.loadMapButton);
    assignUiElement(attractorsButton_, solidGeometry.attractorsButton);
    assignUiElement(selectedColorPreview_, solidGeometry.selectedColorPreview);
    for (std::size_t index = 0; index < stateButtons_.size(); ++index) {
        assignUiElement(stateButtons_[index], solidGeometry.stateButtons[index]);
        assignUiElement(stateSwatches_[index], solidGeometry.stateSwatches[index]);
    }
    for (std::size_t channel = 0; channel < colorAdjustButtons_.size(); ++channel) {
        assignUiElement(colorAdjustButtons_[channel].decrement, solidGeometry.colorAdjustButtons[channel][0]);
        assignUiElement(colorAdjustButtons_[channel].increment, solidGeometry.colorAdjustButtons[channel][1]);
    }
    assignUiElement(attractorWidthButtons_.decrement, solidGeometry.attractorAdjustButtons[0][0]);
    assignUiElement(attractorWidthButtons_.increment, solidGeometry.attractorAdjustButtons[0][1]);
    assignUiElement(attractorHeightButtons_.decrement, solidGeometry.attractorAdjustButtons[1][0]);
    assignUiElement(attractorHeightButtons_.increment, solidGeometry.attractorAdjustButtons[1][1]);
    assignUiElement(attractorRefineButton_, solidGeometry.attractorRefineButton);
    assignUiElement(attractorExportButton_, solidGeometry.attractorExportButton);
    assignUiElement(attractorExportPngButton_, solidGeometry.attractorExportPngButton);

    if (!solidGeometry.vertices.empty()) {
        std::memcpy(
            solidVertexBuffer_.mapped,
            solidGeometry.vertices.data(),
            solidGeometry.vertices.size() * sizeof(SolidVertex));
    }
}

void VulkanApp::updateOverlayTextBuffer() {
    if (overlayTextVertexBuffer_.mapped == nullptr) {
        overlayTextVertexCount_ = 0;
        return;
    }

    const OverlayTextState overlay = viewModel_.overlayText(model_);
    const ClipRect scrollClipRect{
        kSidebarMinX,
        kSidebarScrollAreaMinY,
        kLogicalWindowWidth,
        kSidebarScrollAreaMaxY
    };
    const auto scrollY = [this](const float y) {
        return y - sidebarScrollOffset_;
    };
    std::vector<SolidVertex> vertices;
    vertices.reserve(8192);
    appendText(
        vertices,
        overlay.selectedStateText,
        kTextStartX,
        kStateTextStartY,
        kStateTextPixelSize);
    appendText(
        vertices,
        overlay.generationText,
        kTextStartX,
        kGenerationTextStartY,
        kGenerationTextPixelSize);
    appendText(vertices, "MENU", kSidebarInnerMinX, kSidebarTitleY, 4.0f);
    appendText(vertices, "CARGAR MAPA", 554.0f, 64.0f, 2.3f);
    appendText(vertices, "ATRACTORES", 734.0f, 64.0f, 2.3f);
    appendTextClipped(vertices, overlay.statusText, kSidebarInnerMinX, scrollY(kStatusTextY), 2.4f, scrollClipRect);
    appendTextClipped(vertices, "ESTADOS", kSidebarInnerMinX, scrollY(kStatesTitleY), 3.0f, scrollClipRect);

    for (std::size_t index = 0; index < kStateLabels.size(); ++index) {
        const float rowTextY =
            scrollY(kStateRowStartY + static_cast<float>(index) * (kStateRowHeight + kStateRowGap) + 7.0f);
        appendTextClipped(
            vertices,
            std::to_string(index) + " " + std::string(kStateLabels[index]),
            574.0f,
            rowTextY,
            2.2f,
            scrollClipRect);
    }

    appendTextClipped(vertices, "COLOR RGB", kSidebarInnerMinX, scrollY(kColorEditorTitleY), 3.0f, scrollClipRect);
    appendTextClipped(
        vertices,
        overlay.selectedStateIndexText,
        kSidebarInnerMinX,
        scrollY(kSelectedStateTextY),
        2.4f,
        scrollClipRect);
    appendTextClipped(
        vertices,
        overlay.selectedStateNameText,
        kSidebarInnerMinX,
        scrollY(kSelectedStateTextY + 22.0f),
        2.4f,
        scrollClipRect);

    const std::array<char, 3> channelNames{{'R', 'G', 'B'}};
    for (std::size_t channel = 0; channel < overlay.colorTexts.size(); ++channel) {
        const float rowTextY =
            scrollY(kColorRowStartY + static_cast<float>(channel) * (kColorRowHeight + kColorRowGap) + 8.0f);
        appendTextClipped(vertices, overlay.colorTexts[channel], kSidebarInnerMinX, rowTextY, 2.4f, scrollClipRect);
        appendTextClipped(vertices, std::string(1, channelNames[channel]) + " -", 770.0f, rowTextY, 2.2f, scrollClipRect);
        appendTextClipped(vertices, std::string(1, channelNames[channel]) + " +", 850.0f, rowTextY, 2.2f, scrollClipRect);
    }
    appendTextClipped(vertices, overlay.attractorSizeText, 674.0f, scrollY(kAttractorTextY), 2.2f, scrollClipRect);
    appendTextClipped(vertices, "W -", 600.0f, scrollY(kAttractorButtonMinY + 7.0f), 2.0f, scrollClipRect);
    appendTextClipped(vertices, "W +", 640.0f, scrollY(kAttractorButtonMinY + 7.0f), 2.0f, scrollClipRect);
    appendTextClipped(vertices, "H -", 758.0f, scrollY(kAttractorButtonMinY + 7.0f), 2.0f, scrollClipRect);
    appendTextClipped(vertices, "H +", 798.0f, scrollY(kAttractorButtonMinY + 7.0f), 2.0f, scrollClipRect);
    appendTextClipped(vertices, overlay.attractorSeedText, 546.0f, scrollY(694.0f), 1.8f, scrollClipRect);
    appendTextClipped(vertices, overlay.attractorNodeText, 780.0f, scrollY(694.0f), 1.8f, scrollClipRect);
    appendTextClipped(vertices, overlay.attractorModeText, kSidebarInnerMinX, scrollY(kAttractorModeTextY), 2.2f, scrollClipRect);
    appendTextClipped(vertices, "REFINAR", 656.0f, scrollY(kAttractorRefineButtonMinY + 7.0f), 2.2f, scrollClipRect);
    appendTextClipped(vertices, "EXPORT SVG", 620.0f, scrollY(kAttractorExportButtonMinY + 7.0f), 2.2f, scrollClipRect);
    appendTextClipped(vertices, "EXPORT PNG", 620.0f, scrollY(kAttractorExportPngButtonMinY + 7.0f), 2.2f, scrollClipRect);

    if (vertices.size() > kOverlayTextVertexCapacity) {
        throw std::runtime_error("Overlay text buffer capacity exceeded.");
    }

    std::memcpy(
        overlayTextVertexBuffer_.mapped,
        vertices.data(),
        vertices.size() * sizeof(SolidVertex));
    overlayTextVertexCount_ = static_cast<uint32_t>(vertices.size());
}

VulkanApp::UploadRequest VulkanApp::prepareSimulationUpload(const uint32_t frameIndex) {
    UploadRequest request{};
    if (!simulation_.hasDirtyRegion()) {
        return request;
    }

    const GridSize size = simulation_.gridSize();
    const DirtyRegion dirtyRegion = simulation_.dirtyRegion();
    const uint64_t totalArea = static_cast<uint64_t>(size.w) * static_cast<uint64_t>(size.h);
    const bool shouldUploadFull =
        simulation_.fullUploadRequested() ||
        !dirtyRegion.valid ||
        static_cast<double>(dirtyRegion.area()) > static_cast<double>(totalArea) * kPartialUploadThreshold;

    if (shouldUploadFull) {
        request.mode = UploadMode::Full;
        request.region.valid = true;
        request.region.minX = 0;
        request.region.minY = 0;
        request.region.maxX = size.w - 1U;
        request.region.maxY = size.h - 1U;
        packFullSimulationToStaging(stagingBuffers_[frameIndex].mapped);
    } else {
        request.mode = UploadMode::Partial;
        request.region = dirtyRegion;
        packDirtyRegionToStaging(dirtyRegion, stagingBuffers_[frameIndex].mapped);
    }

    simulation_.clearDirtyTracking();
    return request;
}

void VulkanApp::packFullSimulationToStaging(void* destination) const {
    simulation_.packCombinedStates(destination);
}

void VulkanApp::packDirtyRegionToStaging(const DirtyRegion& region, void* destination) const {
    simulation_.packCombinedStatesRegion(region, destination);
}

void VulkanApp::consumeGpuSimulationResults() {
    if (!simulationGpuEnabled_ || !gpuStepSubmitted_) {
        return;
    }

    throwIfFailed(
        vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX),
        "Failed to wait for simulation results fence");

    const auto* stepStats = static_cast<const uint32_t*>(simulationStatsBuffer_.mapped);
    updateGpuRoutingState(stepStats[0], stepStats[1], stepStats[2]);
    gpuStepSubmitted_ = false;
}

void VulkanApp::updateGpuRoutingState(
    const uint32_t nutrientPending,
    const uint32_t nutrientFound,
    const uint32_t physarumCells) {
    if (nutrientPending == 0U && nutrientFound > 0U) {
        if (static_cast<int>(physarumCells) < gpuPhysarumLastCells_) {
            gpuMinimumPhysarumCells_ = static_cast<int>(physarumCells);
        }

        gpuMinimumCheck_ =
            (gpuMinimumPhysarumCells_ == gpuPhysarumLastCells_) ? (gpuMinimumCheck_ + 1) : 0;
        if (gpuMinimumCheck_ > 10) {
            model_.play() = false;
        }
    }

    gpuPhysarumLastCells_ = static_cast<int>(physarumCells);
}

void VulkanApp::recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    const uint32_t imageIndex,
    const uint32_t frameIndex,
    const UploadRequest& uploadRequest) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    throwIfFailed(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin command buffer recording");

    const GridSize size = simulation_.gridSize();
    uint32_t renderBufferIndex = currentSimulationBufferIndex_;

    if (uploadRequest.mode != UploadMode::None) {
        VkBufferMemoryBarrier toTransferBarrier{};
        toTransferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        toTransferBarrier.srcAccessMask =
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransferBarrier.buffer = simulationStateBuffers_[currentSimulationBufferIndex_].buffer;
        toTransferBarrier.offset = 0;
        toTransferBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            1,
            &toTransferBarrier,
            0,
            nullptr);

        std::vector<VkBufferCopy> copyRegions;
        if (uploadRequest.mode == UploadMode::Full) {
            copyRegions.push_back(VkBufferCopy{
                0,
                0,
                static_cast<VkDeviceSize>(size.w) * static_cast<VkDeviceSize>(size.h) * sizeof(uint32_t)
            });
        } else {
            copyRegions.reserve(uploadRequest.region.height());
            const VkDeviceSize rowBytes = static_cast<VkDeviceSize>(uploadRequest.region.width()) * sizeof(uint32_t);
            for (uint32_t row = 0; row < uploadRequest.region.height(); ++row) {
                copyRegions.push_back(VkBufferCopy{
                    static_cast<VkDeviceSize>(row) * rowBytes,
                    (static_cast<VkDeviceSize>(uploadRequest.region.minY) + row) * static_cast<VkDeviceSize>(size.w) * sizeof(uint32_t) +
                        static_cast<VkDeviceSize>(uploadRequest.region.minX) * sizeof(uint32_t),
                    rowBytes
                });
            }
        }

        vkCmdCopyBuffer(
            commandBuffer,
            stagingBuffers_[frameIndex].buffer,
            simulationStateBuffers_[currentSimulationBufferIndex_].buffer,
            static_cast<uint32_t>(copyRegions.size()),
            copyRegions.data());

        VkBufferMemoryBarrier fromTransferBarrier{};
        fromTransferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        fromTransferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fromTransferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        fromTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fromTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fromTransferBarrier.buffer = simulationStateBuffers_[currentSimulationBufferIndex_].buffer;
        fromTransferBarrier.offset = 0;
        fromTransferBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            pendingGpuSimulationStep_ ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            1,
            &fromTransferBarrier,
            0,
            nullptr);
    }

    if (pendingGpuSimulationStep_ && computePipeline_ != VK_NULL_HANDLE) {
        const uint32_t outputBufferIndex = 1U - currentSimulationBufferIndex_;

        VkBufferMemoryBarrier outputPrepareBarrier{};
        outputPrepareBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        outputPrepareBarrier.srcAccessMask =
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        outputPrepareBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        outputPrepareBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        outputPrepareBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        outputPrepareBarrier.buffer = simulationStateBuffers_[outputBufferIndex].buffer;
        outputPrepareBarrier.offset = 0;
        outputPrepareBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0,
            nullptr,
            1,
            &outputPrepareBarrier,
            0,
            nullptr);

        vkCmdFillBuffer(commandBuffer, simulationStatsBuffer_.buffer, 0, sizeof(uint32_t) * 3U, 0U);

        VkBufferMemoryBarrier statsPrepareBarrier{};
        statsPrepareBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        statsPrepareBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        statsPrepareBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        statsPrepareBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        statsPrepareBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        statsPrepareBarrier.buffer = simulationStatsBuffer_.buffer;
        statsPrepareBarrier.offset = 0;
        statsPrepareBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0,
            nullptr,
            1,
            &statsPrepareBarrier,
            0,
            nullptr);

        struct ComputePushConstants {
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t stepSeed = 0;
            uint32_t reserved = 0;
        } computePush{
            size.w,
            size.h,
            static_cast<uint32_t>(model_.generation()),
            0U
        };

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline_);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            computePipelineLayout_,
            0,
            1,
            &computeDescriptorSets_[currentSimulationBufferIndex_],
            0,
            nullptr);
        vkCmdPushConstants(
            commandBuffer,
            computePipelineLayout_,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(ComputePushConstants),
            &computePush);
        vkCmdDispatch(
            commandBuffer,
            (size.w + 15U) / 16U,
            (size.h + 15U) / 16U,
            1U);

        std::array<VkBufferMemoryBarrier, 2> postComputeBarriers{};
        postComputeBarriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        postComputeBarriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        postComputeBarriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        postComputeBarriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postComputeBarriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postComputeBarriers[0].buffer = simulationStateBuffers_[outputBufferIndex].buffer;
        postComputeBarriers[0].offset = 0;
        postComputeBarriers[0].size = VK_WHOLE_SIZE;

        postComputeBarriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        postComputeBarriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        postComputeBarriers[1].dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        postComputeBarriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postComputeBarriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postComputeBarriers[1].buffer = simulationStatsBuffer_.buffer;
        postComputeBarriers[1].offset = 0;
        postComputeBarriers[1].size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
            0,
            0,
            nullptr,
            static_cast<uint32_t>(postComputeBarriers.size()),
            postComputeBarriers.data(),
            0,
            nullptr);

        renderBufferIndex = outputBufferIndex;
        currentSimulationBufferIndex_ = outputBufferIndex;
        gpuStepSubmitted_ = true;
    }

    VkClearValue clearColor{};
    clearColor.color = {{kClearColor[0], kClearColor[1], kClearColor[2], kClearColor[3]}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = swapChainFramebuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent_;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkDeviceSize zeroOffset = 0;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidPipeline_);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &solidVertexBuffer_.buffer, &zeroOffset);

    const auto isHovered = [this](const UiElement& element) {
        return mouseLogicalX_ >= static_cast<float>(element.rect.x) &&
               mouseLogicalY_ >= static_cast<float>(element.rect.y) &&
               mouseLogicalX_ <= static_cast<float>(element.rect.x + element.rect.width) &&
               mouseLogicalY_ <= static_cast<float>(element.rect.y + element.rect.height);
    };
    const auto drawRange = [&](const SolidDrawRange& range, const std::array<float, 4>& color) {
        RectPushConstants push{};
        std::memcpy(push.color, color.data(), sizeof(push.color));
        vkCmdPushConstants(
            commandBuffer,
            solidPipelineLayout_,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(RectPushConstants),
            &push);
        vkCmdDraw(commandBuffer, range.vertexCount, 1, range.firstVertex, 0);
    };
    const auto drawUiElement = [&](const UiElement& element, const std::array<float, 4>& color) {
        drawRange(element.draw, color);
    };
    const auto rgbaToColor = [](const PhysarumSim::Rgba& rgba) {
        return std::array<float, 4>{
            static_cast<float>(rgba[0]) / 255.0f,
            static_cast<float>(rgba[1]) / 255.0f,
            static_cast<float>(rgba[2]) / 255.0f,
            static_cast<float>(rgba[3]) / 255.0f
        };
    };

    drawRange(panelDraw_, kPanelColor);
    drawRange(sidebarDraw_, kSidebarColor);
    drawUiElement(loadMapButton_, isHovered(loadMapButton_) ? kButtonHoverColor : kButtonColor);
    drawUiElement(attractorsButton_, isHovered(attractorsButton_) ? kButtonHoverColor : kButtonColor);
    drawRange(scrollbarTrackDraw_, kButtonOutlineSoft);
    drawRange(scrollbarThumbDraw_, kButtonHoverColor);

    for (std::size_t index = 0; index < stateButtons_.size(); ++index) {
        std::array<float, 4> rowColor = kButtonColor;
        if (selectedState_ == static_cast<uint8_t>(index)) {
            rowColor = kButtonSelectedColor;
        } else if (isHovered(stateButtons_[index])) {
            rowColor = kButtonHoverColor;
        }
        drawUiElement(stateButtons_[index], rowColor);
        drawUiElement(stateSwatches_[index], rgbaToColor(simulation_.colorForState(static_cast<uint8_t>(index))));
    }

    drawUiElement(selectedColorPreview_, rgbaToColor(simulation_.colorForState(selectedState_)));
    for (std::size_t channel = 0; channel < colorAdjustButtons_.size(); ++channel) {
        const UiElement& decrement = colorAdjustButtons_[channel].decrement;
        const UiElement& increment = colorAdjustButtons_[channel].increment;
        drawUiElement(decrement, isHovered(decrement) ? kButtonHoverColor : kButtonOutlineSoft);
        drawUiElement(increment, isHovered(increment) ? kButtonHoverColor : kButtonOutlineSoft);
    }

    drawUiElement(
        attractorWidthButtons_.decrement,
        isHovered(attractorWidthButtons_.decrement) ? kButtonHoverColor : kButtonOutlineSoft);
    drawUiElement(
        attractorWidthButtons_.increment,
        isHovered(attractorWidthButtons_.increment) ? kButtonHoverColor : kButtonOutlineSoft);
    drawUiElement(
        attractorHeightButtons_.decrement,
        isHovered(attractorHeightButtons_.decrement) ? kButtonHoverColor : kButtonOutlineSoft);
    drawUiElement(
        attractorHeightButtons_.increment,
        isHovered(attractorHeightButtons_.increment) ? kButtonHoverColor : kButtonOutlineSoft);
    const bool canRefineAttractor = viewModel_.canRefineAttractors();
    drawUiElement(
        attractorRefineButton_,
        canRefineAttractor
            ? (isHovered(attractorRefineButton_) ? kButtonHoverColor : kButtonSelectedColor)
            : kButtonOutlineSoft);
    const bool canExportAttractor = latestAttractorGraph_.has_value() && !latestAttractorGraph_->nodes.empty();
    drawUiElement(
        attractorExportButton_,
        canExportAttractor
            ? (isHovered(attractorExportButton_) ? kButtonHoverColor : kButtonSelectedColor)
            : kButtonOutlineSoft);
    const bool canExportPngAttractor = canExportAttractor && AttractorGraphExporter::pngSupported();
    drawUiElement(
        attractorExportPngButton_,
        canExportPngAttractor
            ? (isHovered(attractorExportPngButton_) ? kButtonHoverColor : kButtonSelectedColor)
            : kButtonOutlineSoft);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, texturedPipeline_);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &texturedVertexBuffer_.buffer, &zeroOffset);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        texturedPipelineLayout_,
        0,
        1,
        &descriptorSets_[renderBufferIndex],
        0,
        nullptr);
    const QuadPushConstants quadPush = currentQuadPushConstants();
    vkCmdPushConstants(
        commandBuffer,
        texturedPipelineLayout_,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(QuadPushConstants),
        &quadPush);
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(kSimulationQuadVertices.size()), 1, 0, 0);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, solidPipeline_);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &solidVertexBuffer_.buffer, &zeroOffset);

    drawRange(indicatorDraw_, rgbaToColor(simulation_.colorForState(selectedState_)));

    if (overlayTextVertexCount_ > 0) {
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &overlayTextVertexBuffer_.buffer, &zeroOffset);

        RectPushConstants textPush{};
        std::memcpy(textPush.color, kTextColor.data(), sizeof(textPush.color));
        vkCmdPushConstants(
            commandBuffer,
            solidPipelineLayout_,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(RectPushConstants),
            &textPush);
        vkCmdDraw(commandBuffer, overlayTextVertexCount_, 1, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
    throwIfFailed(vkEndCommandBuffer(commandBuffer), "Failed to record command buffer");
}

void VulkanApp::drawFrame() {
    throwIfFailed(
        vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX),
        "Failed to wait for in-flight fence");

    if (simulationPaletteDirty_ && simulationPaletteBuffer_.mapped != nullptr) {
        auto* paletteWords = static_cast<uint32_t*>(simulationPaletteBuffer_.mapped);
        for (std::size_t index = 0; index < simulation_.palette().size(); ++index) {
            paletteWords[index] = packRgba8(simulation_.palette()[index]);
        }
        simulationPaletteDirty_ = false;
    }

    uint32_t imageIndex = 0;
    const VkResult acquireResult = vkAcquireNextImageKHR(
        device_,
        swapChain_,
        UINT64_MAX,
        imageAvailableSemaphores_[currentFrame_],
        VK_NULL_HANDLE,
        &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image.");
    }

    if (imagesInFlight_[imageIndex] != VK_NULL_HANDLE) {
        throwIfFailed(
            vkWaitForFences(device_, 1, &imagesInFlight_[imageIndex], VK_TRUE, UINT64_MAX),
            "Failed to wait for image-in-flight fence");
    }
    imagesInFlight_[imageIndex] = inFlightFences_[currentFrame_];

    const UploadRequest uploadRequest = prepareSimulationUpload(static_cast<uint32_t>(currentFrame_));
    updateOverlayTextBuffer();

    throwIfFailed(vkResetFences(device_, 1, &inFlightFences_[currentFrame_]), "Failed to reset fence");
    throwIfFailed(vkResetCommandBuffer(commandBuffers_[currentFrame_], 0), "Failed to reset command buffer");

    recordCommandBuffer(
        commandBuffers_[currentFrame_],
        imageIndex,
        static_cast<uint32_t>(currentFrame_),
        uploadRequest);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    {
        std::lock_guard<std::mutex> lock(queueSubmitMutex_);
        throwIfFailed(
            vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]),
            "Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = VK_SUCCESS;
    {
        std::lock_guard<std::mutex> lock(queueSubmitMutex_);
        presentResult = vkQueuePresentKHR(presentQueue_, &presentInfo);
    }
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        framebufferResized_ = false;
        recreateSwapChain();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swapchain image.");
    }

    currentFrame_ = (currentFrame_ + 1U) % kMaxFramesInFlight;
}

SwapChainSupportDetails VulkanApp::querySwapChainSupport(const VkPhysicalDevice deviceHandle) const {
    return querySwapChainSupport(deviceHandle, surface_);
}

SwapChainSupportDetails VulkanApp::querySwapChainSupport(
    const VkPhysicalDevice deviceHandle,
    const VkSurfaceKHR surface) const {
    SwapChainSupportDetails details{};

    throwIfFailed(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceHandle, surface, &details.capabilities),
        "Failed to query surface capabilities");

    uint32_t formatCount = 0;
    throwIfFailed(
        vkGetPhysicalDeviceSurfaceFormatsKHR(deviceHandle, surface, &formatCount, nullptr),
        "Failed to query surface formats");
    if (formatCount > 0) {
        details.formats.resize(formatCount);
        throwIfFailed(
            vkGetPhysicalDeviceSurfaceFormatsKHR(deviceHandle, surface, &formatCount, details.formats.data()),
            "Failed to query surface formats");
    }

    uint32_t presentModeCount = 0;
    throwIfFailed(
        vkGetPhysicalDeviceSurfacePresentModesKHR(deviceHandle, surface, &presentModeCount, nullptr),
        "Failed to query surface present modes");
    if (presentModeCount > 0) {
        details.presentModes.resize(presentModeCount);
        throwIfFailed(
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                deviceHandle,
                surface,
                &presentModeCount,
                details.presentModes.data()),
            "Failed to query surface present modes");
    }

    return details;
}

QueueFamilyIndices VulkanApp::findQueueFamilies(const VkPhysicalDevice deviceHandle) const {
    QueueFamilyIndices indices{};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(deviceHandle, &queueFamilyCount, queueFamilies.data());

    for (uint32_t index = 0; index < queueFamilyCount; ++index) {
        if ((queueFamilies[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
            indices.graphicsFamily = index;
        }
        if (!indices.computeFamily.has_value() &&
            (queueFamilies[index].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U) {
            indices.computeFamily = index;
        }

        VkBool32 presentSupport = VK_FALSE;
        throwIfFailed(
            vkGetPhysicalDeviceSurfaceSupportKHR(deviceHandle, index, surface_, &presentSupport),
            "Failed to query present support");
        if (presentSupport == VK_TRUE) {
            indices.presentFamily = index;
        }

        if (indices.isComplete()) {
            break;
        }
    }

    return indices;
}

bool VulkanApp::checkDeviceExtensionSupport(const VkPhysicalDevice deviceHandle) const {
    uint32_t extensionCount = 0;
    throwIfFailed(
        vkEnumerateDeviceExtensionProperties(deviceHandle, nullptr, &extensionCount, nullptr),
        "Failed to enumerate device extensions");

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    throwIfFailed(
        vkEnumerateDeviceExtensionProperties(deviceHandle, nullptr, &extensionCount, availableExtensions.data()),
        "Failed to enumerate device extensions");

    std::set<std::string> requiredExtensions(requiredDeviceExtensions_.begin(), requiredDeviceExtensions_.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool VulkanApp::checkValidationLayerSupport() const {
    uint32_t layerCount = 0;
    throwIfFailed(vkEnumerateInstanceLayerProperties(&layerCount, nullptr), "Failed to enumerate validation layers");

    std::vector<VkLayerProperties> availableLayers(layerCount);
    throwIfFailed(
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()),
        "Failed to enumerate validation layers");

    for (const char* layerName : validationLayers_) {
        bool found = false;
        for (const auto& layerProperties : availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> VulkanApp::getRequiredExtensions() const {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions == nullptr || glfwExtensionCount == 0) {
        throw std::runtime_error("GLFW did not return required Vulkan instance extensions.");
    }

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (validationLayersEnabled_) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

VkSurfaceFormatKHR VulkanApp::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats.front();
}

VkPresentModeKHR VulkanApp::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) const {
    for (const VkPresentModeKHR availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    for (const VkPresentModeKHR availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            return availablePresentMode;
        }
    }

    for (const VkPresentModeKHR availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    return chooseSwapExtent(capabilities, window_);
}

VkExtent2D VulkanApp::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities,
    GLFWwindow* targetWindow) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(targetWindow, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(
        actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return actualExtent;
}

uint32_t VulkanApp::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);

    for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        const bool matchesType = (typeFilter & (1U << index)) != 0U;
        const bool matchesProperties =
            (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties;
        if (matchesType && matchesProperties) {
            return index;
        }
    }

    throw std::runtime_error("Failed to find suitable Vulkan memory type.");
}

VkShaderModule VulkanApp::createShaderModule(const std::vector<char>& code) const {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    throwIfFailed(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule), "Failed to create shader module");
    return shaderModule;
}

VkImageView VulkanApp::createImageView(const VkImage image, const VkFormat format) const {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = VK_NULL_HANDLE;
    throwIfFailed(vkCreateImageView(device_, &viewInfo, nullptr, &imageView), "Failed to create image view");
    return imageView;
}

std::filesystem::path VulkanApp::shaderPath(const std::string& name) const {
    const std::filesystem::path fromExecutable = executableDirectory() / "shaders" / name;
    if (std::filesystem::exists(fromExecutable)) {
        return fromExecutable;
    }

    const std::filesystem::path fromCurrentDirectory = std::filesystem::current_path() / "shaders" / name;
    if (std::filesystem::exists(fromCurrentDirectory)) {
        return fromCurrentDirectory;
    }

    return fromExecutable;
}

void VulkanApp::createBuffer(
    const VkDeviceSize size,
    const VkBufferUsageFlags usage,
    const VkMemoryPropertyFlags properties,
    BufferAllocation& allocation) {
    allocation.size = size;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throwIfFailed(vkCreateBuffer(device_, &bufferInfo, nullptr, &allocation.buffer), "Failed to create buffer");

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device_, allocation.buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    throwIfFailed(vkAllocateMemory(device_, &allocInfo, nullptr, &allocation.memory), "Failed to allocate buffer memory");
    throwIfFailed(vkBindBufferMemory(device_, allocation.buffer, allocation.memory, 0), "Failed to bind buffer memory");
}

void VulkanApp::createImage(
    const uint32_t width,
    const uint32_t height,
    const VkFormat format,
    const VkImageUsageFlags usage,
    const VkMemoryPropertyFlags properties,
    ImageAllocation& allocation) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throwIfFailed(vkCreateImage(device_, &imageInfo, nullptr, &allocation.image), "Failed to create image");

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(device_, allocation.image, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    throwIfFailed(vkAllocateMemory(device_, &allocInfo, nullptr, &allocation.memory), "Failed to allocate image memory");
    throwIfFailed(vkBindImageMemory(device_, allocation.image, allocation.memory, 0), "Failed to bind image memory");
}

void VulkanApp::copyBuffer(const BufferAllocation& source, const BufferAllocation& destination, const VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, source.buffer, destination.buffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanApp::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    throwIfFailed(vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer), "Failed to allocate command buffer");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    throwIfFailed(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin command buffer");
    return commandBuffer;
}

void VulkanApp::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    throwIfFailed(vkEndCommandBuffer(commandBuffer), "Failed to end command buffer");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    {
        std::lock_guard<std::mutex> lock(queueSubmitMutex_);
        throwIfFailed(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit command buffer");
        throwIfFailed(vkQueueWaitIdle(graphicsQueue_), "Failed to wait for graphics queue idle");
    }

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void VulkanApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int, int) {
    auto* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app != nullptr) {
        app->framebufferResized_ = true;
    }
}

void VulkanApp::attractorFramebufferResizeCallback(GLFWwindow* window, int, int) {
    auto* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app != nullptr && app->attractorWindow_.window == window) {
        app->attractorWindow_.framebufferResized = true;
    }
}

void VulkanApp::scrollCallback(GLFWwindow* window, double, double yOffset) {
    auto* app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app == nullptr) {
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    const auto [logicalMouseX, logicalMouseY] = app->screenToLogical(mouseX, mouseY);
    if (app->isInsideSidebarScrollableArea(logicalMouseX, logicalMouseY)) {
        app->nudgeSidebarScroll(static_cast<float>(-yOffset) * kSidebarScrollStep);
        return;
    }

    const bool ctrlPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    if (!ctrlPressed) {
        return;
    }

    app->pendingZoomDelta_ += yOffset;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void*) {
    std::cerr << "validation layer: " << callbackData->pMessage << '\n';
    return VK_FALSE;
}
