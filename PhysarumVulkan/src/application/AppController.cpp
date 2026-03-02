#include "application/AppController.h"

#include <algorithm>
#include <exception>
#include <string_view>
#include <utility>

AppController::AppController(AppModel& model, AppViewModel& viewModel)
    : model_(model),
      viewModel_(viewModel) {
}

void AppController::initializeSimulationClock(const std::chrono::steady_clock::time_point now) {
    model_.lastSimulationStep() = now;
}

void AppController::shutdown() {
    attractorGenerator_.shutdown();
}

void AppController::togglePlayback() {
    model_.togglePlayback();
}

void AppController::pauseSimulation() {
    model_.pause();
}

void AppController::selectState(const uint8_t state) {
    viewModel_.selectState(state);
}

void AppController::paintSelectedState(const uint32_t cellX, const uint32_t cellY) {
    model_.simulation().setCellState(cellX, cellY, viewModel_.selectedState());
}

bool AppController::advanceSimulation(
    const std::chrono::steady_clock::time_point now,
    const std::chrono::milliseconds stepInterval) {
    return model_.advanceSimulation(now, stepInterval);
}

void AppController::setMenuStatus(const MenuStatus status) {
    viewModel_.menuStatus() = status;
}

void AppController::setAttractorBackendStatus(std::string status) {
    viewModel_.attractorComputeStatus() = std::move(status);
}

void AppController::setAttractorError() {
    viewModel_.menuStatus() = MenuStatus::AttractorsError;
}

void AppController::notifyAttractorPreviewClosed() {
    viewModel_.showingAttractorGraph() = false;
}

void AppController::loadMapFromImage(
    const std::filesystem::path& imagePath,
    const std::chrono::steady_clock::time_point now) {
    model_.stopAndReset(now);
    resetAttractorSession(true);

    try {
        model_.simulation().loadMapFromImage(imagePath);
        viewModel_.menuStatus() = MenuStatus::MapLoaded;
    } catch (const std::exception& exception) {
        const std::string_view what = exception.what();
        if (what.find("OpenCV support") != std::string_view::npos) {
            viewModel_.menuStatus() = MenuStatus::OpenCvUnavailable;
        } else {
            viewModel_.menuStatus() = MenuStatus::MapError;
        }
    }
}

void AppController::nudgeSelectedStateColor(const std::size_t channel, const int delta) {
    if (channel >= 3) {
        return;
    }

    PhysarumSim::Rgba color = model_.simulation().colorForState(viewModel_.selectedState());
    const int nextValue = std::clamp(static_cast<int>(color[channel]) + delta, 0, 255);
    color[channel] = static_cast<uint8_t>(nextValue);
    model_.simulation().setPaletteColor(viewModel_.selectedState(), color);
    viewModel_.menuStatus() = MenuStatus::Ready;
}

bool AppController::nudgeAttractorDimension(const bool adjustWidth, const int delta) {
    if (viewModel_.attractorProgress().running) {
        return false;
    }

    AttractorSettings candidate = viewModel_.attractorSettings();
    uint32_t& dimension = adjustWidth ? candidate.width : candidate.height;
    const int nextValue = std::clamp(static_cast<int>(dimension) + delta, 1, 5);
    dimension = static_cast<uint32_t>(nextValue);

    viewModel_.attractorSettings() = candidate;
    resetAttractorSession(true);
    viewModel_.menuStatus() = MenuStatus::AttractorsPending;
    return true;
}

GridResizeResult AppController::resizeGrid(const GridSize newSize, const std::chrono::steady_clock::time_point now) {
    const GridSize currentSize = model_.simulation().gridSize();

    model_.stopAndReset(now);
    resetAttractorSession(true);
    viewModel_.menuStatus() = MenuStatus::Ready;
    model_.resizeGrid(newSize);

    return GridResizeResult{
        currentSize.w != newSize.w || currentSize.h != newSize.h
    };
}

AttractorPreviewRequest AppController::toggleAttractorGraph(BatchSuccessorEvaluator successorEvaluator) {
    AttractorPreviewRequest request;
    AttractorProgress& progress = viewModel_.attractorProgress();

    if (progress.running) {
        return request;
    }

    if (viewModel_.showingAttractorGraph() && progress.completed && !progress.hasError) {
        viewModel_.showingAttractorGraph() = false;
        viewModel_.menuStatus() = MenuStatus::Ready;
        request.closePreview = true;
        return request;
    }

    model_.pause();
    viewModel_.latestAttractorGraph().reset();
    viewModel_.showingAttractorGraph() = true;

    if (attractorGenerator_.start(viewModel_.attractorSettings(), std::move(successorEvaluator), false)) {
        progress = attractorGenerator_.progress();
        viewModel_.menuStatus() = MenuStatus::AttractorsRunning;
        request.openPreview = true;
        request.clearRenderedGraph = true;
        request.resetPreviewBounds = true;
        request.statusText = "Generando vista del atractor...";
        return request;
    }

    viewModel_.showingAttractorGraph() = false;
    viewModel_.menuStatus() = MenuStatus::AttractorsError;
    request.closePreview = true;
    return request;
}

AttractorPreviewRequest AppController::refineAttractorGraph(BatchSuccessorEvaluator successorEvaluator) {
    AttractorPreviewRequest request;
    if (!viewModel_.canRefineAttractors()) {
        return request;
    }

    if (attractorGenerator_.start(viewModel_.attractorSettings(), std::move(successorEvaluator), true)) {
        viewModel_.showingAttractorGraph() = true;
        viewModel_.attractorProgress() = attractorGenerator_.progress();
        viewModel_.menuStatus() = MenuStatus::AttractorsRunning;
        request.openPreview = true;
        request.statusText = "Refinando vista del atractor...";
        return request;
    }

    viewModel_.menuStatus() = MenuStatus::AttractorsError;
    return request;
}

AttractorUpdate AppController::pollAttractorUpdates() {
    AttractorUpdate update;
    viewModel_.attractorProgress() = attractorGenerator_.progress();

    std::optional<AttractorGraph> graph = attractorGenerator_.takeLatestGraph();
    if (graph.has_value()) {
        viewModel_.latestAttractorGraph() = std::move(graph.value());
        update.graph = &viewModel_.latestAttractorGraph().value();
    }

    viewModel_.synchronizeAttractorStatus();
    return update;
}

bool AppController::canRefineAttractors() const {
    return viewModel_.canRefineAttractors();
}

bool AppController::hasExportableAttractorGraph() const {
    return viewModel_.latestAttractorGraph().has_value() &&
           !viewModel_.latestAttractorGraph()->nodes.empty();
}

const std::optional<AttractorGraph>& AppController::latestAttractorGraph() const {
    return viewModel_.latestAttractorGraph();
}

void AppController::resetAttractorSession(const bool hidePreview) {
    attractorGenerator_.cancel();
    viewModel_.attractorProgress() = {};
    viewModel_.latestAttractorGraph().reset();

    if (hidePreview) {
        viewModel_.showingAttractorGraph() = false;
    }
}
