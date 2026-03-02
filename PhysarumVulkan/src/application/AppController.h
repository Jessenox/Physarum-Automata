#pragma once

#include "AppModel.h"
#include "AppViewModel.h"
#include "AttractorGenerator.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

struct GridResizeResult {
    bool dimensionsChanged = false;
};

struct AttractorPreviewRequest {
    bool openPreview = false;
    bool closePreview = false;
    bool clearRenderedGraph = false;
    bool resetPreviewBounds = false;
    std::string statusText;
};

struct AttractorUpdate {
    const AttractorGraph* graph = nullptr;
};

class AppController {
public:
    AppController(AppModel& model, AppViewModel& viewModel);

    [[nodiscard]] AppModel& model() {
        return model_;
    }

    [[nodiscard]] const AppModel& model() const {
        return model_;
    }

    [[nodiscard]] AppViewModel& viewModel() {
        return viewModel_;
    }

    [[nodiscard]] const AppViewModel& viewModel() const {
        return viewModel_;
    }

    [[nodiscard]] PhysarumSim& simulation() {
        return model_.simulation();
    }

    [[nodiscard]] const PhysarumSim& simulation() const {
        return model_.simulation();
    }

    void initializeSimulationClock(std::chrono::steady_clock::time_point now);
    void shutdown();

    void togglePlayback();
    void pauseSimulation();
    void selectState(uint8_t state);
    void paintSelectedState(uint32_t cellX, uint32_t cellY);
    bool advanceSimulation(
        std::chrono::steady_clock::time_point now,
        std::chrono::milliseconds stepInterval);

    void setMenuStatus(MenuStatus status);
    void setAttractorBackendStatus(std::string status);
    void setAttractorError();
    void notifyAttractorPreviewClosed();

    void loadMapFromImage(
        const std::filesystem::path& imagePath,
        std::chrono::steady_clock::time_point now);
    void nudgeSelectedStateColor(std::size_t channel, int delta);
    bool nudgeAttractorDimension(bool adjustWidth, int delta);
    GridResizeResult resizeGrid(GridSize newSize, std::chrono::steady_clock::time_point now);

    [[nodiscard]] AttractorPreviewRequest toggleAttractorGraph(BatchSuccessorEvaluator successorEvaluator);
    [[nodiscard]] AttractorPreviewRequest refineAttractorGraph(BatchSuccessorEvaluator successorEvaluator);
    [[nodiscard]] AttractorUpdate pollAttractorUpdates();

    [[nodiscard]] bool canRefineAttractors() const;
    [[nodiscard]] bool hasExportableAttractorGraph() const;
    [[nodiscard]] const std::optional<AttractorGraph>& latestAttractorGraph() const;

private:
    void resetAttractorSession(bool hidePreview);

private:
    AppModel& model_;
    AppViewModel& viewModel_;
    AttractorGenerator attractorGenerator_{};
};
