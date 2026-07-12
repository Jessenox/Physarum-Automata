#include "presentation/mvvm/AppViewModel.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace {

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

}  // namespace

void AppViewModel::selectState(const uint8_t state) {
    selectedState_ = static_cast<uint8_t>(std::min<std::size_t>(state, kStateLabels.size() - 1U));
}

void AppViewModel::synchronizeAttractorStatus() {
    if (attractorProgress_.running) {
        menuStatus_ = MenuStatus::AttractorsRunning;
        return;
    }

    if (attractorProgress_.completed && attractorProgress_.hasError) {
        menuStatus_ = MenuStatus::AttractorsError;
        return;
    }

    if (attractorProgress_.completed && !attractorProgress_.hasError && !isExportStatus(menuStatus_)) {
        menuStatus_ = MenuStatus::AttractorsReady;
    }
}

std::string AppViewModel::menuStatusText() const {
    switch (menuStatus_) {
        case MenuStatus::Ready:
            return "LISTO";
        case MenuStatus::MapLoaded:
            return "MAPA LISTO";
        case MenuStatus::MapError:
            return "MAPA ERROR";
        case MenuStatus::DialogUnavailable:
            return "DIALOGO NO";
        case MenuStatus::OpenCvUnavailable:
            return "OPENCV OFF";
        case MenuStatus::AttractorsPending:
            return "ATR CONFIG";
        case MenuStatus::AttractorsRunning:
            return "ATR RUN";
        case MenuStatus::AttractorsReady:
            return "ATR LISTO";
        case MenuStatus::AttractorsError:
            return "ATR ERROR";
        case MenuStatus::SvgExported:
            return "SVG LISTO";
        case MenuStatus::SvgExportError:
            return "SVG ERROR";
        case MenuStatus::PngExported:
            return "PNG LISTO";
        case MenuStatus::PngExportError:
            return "PNG ERROR";
    }

    return "LISTO";
}

std::string AppViewModel::windowTitle(const AppModel& model, const float zoom) const {
    const GridSize size = model.requestedGridSize();

    std::ostringstream title;
    title << "PhysarumVulkan | Generation: " << model.generation()
          << " | State: " << static_cast<int>(selectedState_)
          << " | Grid: " << size.w << 'x' << size.h
          << " | Zoom: " << std::fixed << std::setprecision(1) << zoom << "x"
          << " | " << (model.play() ? "Playing" : "Paused");

    if (attractorProgress_.running) {
        title << " | Attractor "
              << (attractorProgress_.approximate ? "APROX " : "EXACT ")
              << attractorProgress_.processedSeeds << '/' << attractorProgress_.totalSeeds
              << " nodes " << attractorProgress_.discoveredNodes
              << ' ' << attractorComputeStatus_;
    } else if (showingAttractorGraph_) {
        title << " | Attractor View "
              << attractorSettings_.width << 'x' << attractorSettings_.height
              << ' ' << (usesApproximateAttractorMode(attractorSettings_) ? "APROX" : "EXACT")
              << ' ' << attractorComputeStatus_;
    }

    return title.str();
}

OverlayTextState AppViewModel::overlayText(const AppModel& model) const {
    const PhysarumSim::Rgba selectedColor = model.simulation().colorForState(selectedState_);
    const bool approximateAttractors = usesApproximateAttractorMode(attractorSettings_);

    OverlayTextState text;
    text.statusText = menuStatusText();
    text.selectedStateText = "STATE " + std::to_string(selectedState_);
    text.generationText = "GEN " + std::to_string(model.generation());
    text.selectedStateIndexText = "SELEC " + std::to_string(selectedState_);
    text.selectedStateNameText = std::string(stateLabel(selectedState_));
    text.colorTexts = {{
        "R " + std::to_string(selectedColor[0]),
        "G " + std::to_string(selectedColor[1]),
        "B " + std::to_string(selectedColor[2]),
    }};
    text.attractorSizeText =
        "ATR " + std::to_string(attractorSettings_.width) + "X" + std::to_string(attractorSettings_.height);
    text.attractorModeText = approximateAttractors ? "MODO APROX" : "MODO EXACTO";
    text.attractorSeedText =
        std::string(approximateAttractors ? "MUE " : "SEM ") +
        std::to_string(attractorProgress_.processedSeeds) + " OF " + std::to_string(attractorProgress_.totalSeeds);
    text.attractorNodeText = "NOD " + std::to_string(attractorProgress_.discoveredNodes);
    return text;
}

bool AppViewModel::canRefineAttractors() const {
    return attractorProgress_.completed &&
           !attractorProgress_.hasError &&
           !attractorProgress_.running &&
           attractorProgress_.canRefine;
}

bool AppViewModel::usesApproximateAttractorMode(const AttractorSettings& settings) {
    return settings.width * settings.height > 9U;
}

std::string_view AppViewModel::stateLabel(const std::size_t index) {
    return kStateLabels.at(std::min<std::size_t>(index, kStateLabels.size() - 1U));
}

bool AppViewModel::isExportStatus(const MenuStatus status) {
    return status == MenuStatus::SvgExported ||
           status == MenuStatus::SvgExportError ||
           status == MenuStatus::PngExported ||
           status == MenuStatus::PngExportError;
}
