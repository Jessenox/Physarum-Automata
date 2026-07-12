#include "presentation/ui/ViewTransform.h"

#include <algorithm>
#include <cmath>

ViewTransform::ViewTransform(Config config)
    : config_(config),
      zoom_(config.minZoom) {
}

void ViewTransform::reset() {
    zoom_ = config_.minZoom;
    centerX_ = 0.5f;
    centerY_ = 0.5f;
    panVelocityX_ = 0.0f;
    panVelocityY_ = 0.0f;
}

void ViewTransform::clamp() {
    const float visibleWidth = 1.0f / zoom_;
    const float visibleHeight = 1.0f / zoom_;
    const float halfWidth = visibleWidth * 0.5f;
    const float halfHeight = visibleHeight * 0.5f;

    centerX_ = std::clamp(centerX_, halfWidth, 1.0f - halfWidth);
    centerY_ = std::clamp(centerY_, halfHeight, 1.0f - halfHeight);
}

void ViewTransform::zoomAtCursor(
    const double mouseX,
    const double mouseY,
    const ViewportRect& viewport,
    const float zoomMultiplier) {
    if (viewport.width <= 0.0 || viewport.height <= 0.0) {
        return;
    }

    const float beforeSpan = 1.0f / zoom_;
    const float cursorU = static_cast<float>(
        std::clamp((mouseX - viewport.x) / viewport.width, 0.0, 1.0));
    const float cursorV = static_cast<float>(
        std::clamp((mouseY - viewport.y) / viewport.height, 0.0, 1.0));
    const float beforeMinX = centerX_ - beforeSpan * 0.5f;
    const float beforeMinY = centerY_ - beforeSpan * 0.5f;
    const float focusU = beforeMinX + beforeSpan * cursorU;
    const float focusV = beforeMinY + beforeSpan * cursorV;

    zoom_ = std::clamp(zoom_ * zoomMultiplier, config_.minZoom, config_.maxZoom);

    const float afterSpan = 1.0f / zoom_;
    centerX_ = focusU + (0.5f - cursorU) * afterSpan;
    centerY_ = focusV + (0.5f - cursorV) * afterSpan;

    clamp();
}

bool ViewTransform::updatePan(
    const double mouseX,
    const double mouseY,
    const ViewportRect& viewport,
    const bool panRequestedInsideViewport) {
    if (!panRequestedInsideViewport) {
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

    if (viewport.width <= 0.0 || viewport.height <= 0.0) {
        return false;
    }

    const float visibleSpan = 1.0f / zoom_;
    const float desiredVelocityX = -static_cast<float>(deltaX / viewport.width) * visibleSpan;
    const float desiredVelocityY = -static_cast<float>(deltaY / viewport.height) * visibleSpan;
    panVelocityX_ =
        panVelocityX_ * config_.panBlendFactor + desiredVelocityX * (1.0f - config_.panBlendFactor);
    panVelocityY_ =
        panVelocityY_ * config_.panBlendFactor + desiredVelocityY * (1.0f - config_.panBlendFactor);
    return true;
}

void ViewTransform::updateMotion() {
    if (std::abs(panVelocityX_) <= config_.panVelocityEpsilon) {
        panVelocityX_ = 0.0f;
    }
    if (std::abs(panVelocityY_) <= config_.panVelocityEpsilon) {
        panVelocityY_ = 0.0f;
    }
    if (panVelocityX_ == 0.0f && panVelocityY_ == 0.0f) {
        return;
    }

    const float unclampedCenterX = centerX_ + panVelocityX_;
    const float unclampedCenterY = centerY_ + panVelocityY_;
    centerX_ = unclampedCenterX;
    centerY_ = unclampedCenterY;
    clamp();

    if (centerX_ != unclampedCenterX) {
        panVelocityX_ = 0.0f;
    }
    if (centerY_ != unclampedCenterY) {
        panVelocityY_ = 0.0f;
    }

    if (!panActive_) {
        panVelocityX_ *= config_.panInertiaDamping;
        panVelocityY_ *= config_.panInertiaDamping;
    }
}

ViewBounds ViewTransform::bounds() const {
    const float visibleSpan = 1.0f / zoom_;
    const float halfSpan = visibleSpan * 0.5f;
    return ViewBounds{
        centerX_ - halfSpan,
        centerY_ - halfSpan,
        centerX_ + halfSpan,
        centerY_ + halfSpan
    };
}

std::pair<uint32_t, uint32_t> ViewTransform::screenToCell(
    const double mouseX,
    const double mouseY,
    const ViewportRect& viewport,
    const ViewGridExtent gridExtent) const {
    if (viewport.width <= 0.0 || viewport.height <= 0.0 || gridExtent.width == 0 || gridExtent.height == 0) {
        return {0, 0};
    }

    const ViewBounds view = bounds();
    const double normalizedX = std::clamp((mouseX - viewport.x) / viewport.width, 0.0, 1.0);
    const double normalizedY = std::clamp((mouseY - viewport.y) / viewport.height, 0.0, 1.0);
    const double u = std::clamp(
        static_cast<double>(view.minX) + normalizedX * static_cast<double>(view.maxX - view.minX),
        0.0,
        0.999999);
    const double v = std::clamp(
        static_cast<double>(view.minY) + normalizedY * static_cast<double>(view.maxY - view.minY),
        0.0,
        0.999999);

    const uint32_t cellX = std::min<uint32_t>(
        static_cast<uint32_t>(u * static_cast<double>(gridExtent.width)),
        gridExtent.width - 1U);
    const uint32_t cellY = std::min<uint32_t>(
        static_cast<uint32_t>(v * static_cast<double>(gridExtent.height)),
        gridExtent.height - 1U);
    return {cellX, cellY};
}

bool ViewTransform::hasPanVelocity() const {
    return std::abs(panVelocityX_) > config_.panVelocityEpsilon ||
           std::abs(panVelocityY_) > config_.panVelocityEpsilon;
}
