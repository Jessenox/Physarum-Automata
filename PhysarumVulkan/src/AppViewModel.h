#pragma once

#include "AppModel.h"
#include "AttractorGenerator.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

enum class MenuStatus {
    Ready,
    MapLoaded,
    MapError,
    DialogUnavailable,
    OpenCvUnavailable,
    AttractorsPending,
    AttractorsRunning,
    AttractorsReady,
    AttractorsError,
    SvgExported,
    SvgExportError,
    PngExported,
    PngExportError,
};

struct OverlayTextState {
    std::string statusText = "LISTO";
    std::string selectedStateText;
    std::string generationText;
    std::string selectedStateIndexText;
    std::string selectedStateNameText;
    std::array<std::string, 3> colorTexts{};
    std::string attractorSizeText;
    std::string attractorModeText;
    std::string attractorSeedText;
    std::string attractorNodeText;
};

class AppViewModel {
public:
    AppViewModel() = default;

    [[nodiscard]] uint8_t& selectedState() {
        return selectedState_;
    }

    [[nodiscard]] const uint8_t& selectedState() const {
        return selectedState_;
    }

    [[nodiscard]] float& sidebarScrollOffset() {
        return sidebarScrollOffset_;
    }

    [[nodiscard]] const float& sidebarScrollOffset() const {
        return sidebarScrollOffset_;
    }

    [[nodiscard]] MenuStatus& menuStatus() {
        return menuStatus_;
    }

    [[nodiscard]] const MenuStatus& menuStatus() const {
        return menuStatus_;
    }

    [[nodiscard]] bool& showingAttractorGraph() {
        return showingAttractorGraph_;
    }

    [[nodiscard]] const bool& showingAttractorGraph() const {
        return showingAttractorGraph_;
    }

    [[nodiscard]] AttractorSettings& attractorSettings() {
        return attractorSettings_;
    }

    [[nodiscard]] const AttractorSettings& attractorSettings() const {
        return attractorSettings_;
    }

    [[nodiscard]] AttractorProgress& attractorProgress() {
        return attractorProgress_;
    }

    [[nodiscard]] const AttractorProgress& attractorProgress() const {
        return attractorProgress_;
    }

    [[nodiscard]] std::optional<AttractorGraph>& latestAttractorGraph() {
        return latestAttractorGraph_;
    }

    [[nodiscard]] const std::optional<AttractorGraph>& latestAttractorGraph() const {
        return latestAttractorGraph_;
    }

    [[nodiscard]] std::string& attractorComputeStatus() {
        return attractorComputeStatus_;
    }

    [[nodiscard]] const std::string& attractorComputeStatus() const {
        return attractorComputeStatus_;
    }

    void selectState(uint8_t state);
    void synchronizeAttractorStatus();
    [[nodiscard]] std::string menuStatusText() const;
    [[nodiscard]] std::string windowTitle(const AppModel& model, float zoom) const;
    [[nodiscard]] OverlayTextState overlayText(const AppModel& model) const;
    [[nodiscard]] bool canRefineAttractors() const;

    [[nodiscard]] static bool usesApproximateAttractorMode(const AttractorSettings& settings);
    [[nodiscard]] static std::string_view stateLabel(std::size_t index);

private:
    static bool isExportStatus(MenuStatus status);

private:
    uint8_t selectedState_ = 0;
    float sidebarScrollOffset_ = 0.0f;
    MenuStatus menuStatus_ = MenuStatus::Ready;
    bool showingAttractorGraph_ = false;
    AttractorSettings attractorSettings_{};
    AttractorProgress attractorProgress_{};
    std::optional<AttractorGraph> latestAttractorGraph_{};
    std::string attractorComputeStatus_ = "ATR CPU";
};
