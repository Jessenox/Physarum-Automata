#pragma once

#include <cstdint>
#include <utility>

struct ViewportRect {
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

struct ViewBounds {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 1.0f;
    float maxY = 1.0f;
};

struct ViewGridExtent {
    uint32_t width = 0;
    uint32_t height = 0;
};

class ViewTransform {
public:
    struct Config {
        float minZoom = 1.0f;
        float maxZoom = 128.0f;
        float panBlendFactor = 0.35f;
        float panInertiaDamping = 0.82f;
        float panVelocityEpsilon = 0.00002f;
    };

    explicit ViewTransform(Config config);

    void reset();
    void clamp();
    void zoomAtCursor(double mouseX, double mouseY, const ViewportRect& viewport, float zoomMultiplier);
    [[nodiscard]] bool updatePan(double mouseX, double mouseY, const ViewportRect& viewport, bool panRequestedInsideViewport);
    void updateMotion();

    [[nodiscard]] ViewBounds bounds() const;
    [[nodiscard]] std::pair<uint32_t, uint32_t> screenToCell(
        double mouseX,
        double mouseY,
        const ViewportRect& viewport,
        ViewGridExtent gridExtent) const;

    [[nodiscard]] float zoom() const {
        return zoom_;
    }

    [[nodiscard]] bool panActive() const {
        return panActive_;
    }

    [[nodiscard]] bool hasPanVelocity() const;

private:
    Config config_{};
    float zoom_ = 1.0f;
    float centerX_ = 0.5f;
    float centerY_ = 0.5f;
    bool panActive_ = false;
    double lastPanMouseX_ = 0.0;
    double lastPanMouseY_ = 0.0;
    float panVelocityX_ = 0.0f;
    float panVelocityY_ = 0.0f;
};
