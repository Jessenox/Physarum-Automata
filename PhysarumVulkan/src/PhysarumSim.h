#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

struct GridSize {
    uint32_t w = 200;
    uint32_t h = 200;
};

struct DirtyRegion {
    bool valid = false;
    uint32_t minX = 0;
    uint32_t minY = 0;
    uint32_t maxX = 0;
    uint32_t maxY = 0;

    [[nodiscard]] uint32_t width() const {
        return valid ? (maxX - minX + 1U) : 0U;
    }

    [[nodiscard]] uint32_t height() const {
        return valid ? (maxY - minY + 1U) : 0U;
    }

    [[nodiscard]] uint64_t area() const {
        return static_cast<uint64_t>(width()) * static_cast<uint64_t>(height());
    }
};

class PhysarumSim {
public:
    using Rgba = std::array<uint8_t, 4>;
    using Palette = std::array<Rgba, 9>;

    explicit PhysarumSim(GridSize size);

    void resizeGrid(GridSize newSize);
    void setCellState(uint32_t x, uint32_t y, uint16_t value);
    void setPaletteColor(uint8_t state, Rgba color);
    void loadMapFromImage(const std::filesystem::path& imagePath);
    void evaluatePhysarum();
    void clearDirtyTracking();

    [[nodiscard]] GridSize gridSize() const {
        return gridSize_;
    }

    [[nodiscard]] const std::vector<uint8_t>& rgbaPixels() const {
        return rgbaPixels_;
    }

    [[nodiscard]] const DirtyRegion& dirtyRegion() const {
        return dirtyRegion_;
    }

    [[nodiscard]] bool hasDirtyRegion() const {
        return forceFullUpload_ || dirtyRegion_.valid;
    }

    [[nodiscard]] bool fullUploadRequested() const {
        return forceFullUpload_;
    }

    [[nodiscard]] bool routed() const {
        return routed_;
    }

    [[nodiscard]] const Palette& palette() const {
        return palette_;
    }

    [[nodiscard]] Rgba colorForState(uint8_t state) const;
    [[nodiscard]] static const Palette& defaultPalette();

private:
    class Matrix {
    public:
        Matrix() = default;
        explicit Matrix(GridSize size) {
            resize(size);
        }

        void resize(GridSize size);
        [[nodiscard]] uint16_t get(uint32_t x, uint32_t y) const;
        [[nodiscard]] uint16_t& at(uint32_t x, uint32_t y);
        void set(uint32_t x, uint32_t y, uint16_t value);
        [[nodiscard]] GridSize size() const {
            return size_;
        }

    private:
        [[nodiscard]] std::size_t indexOf(uint32_t x, uint32_t y) const;

        GridSize size_{};
        std::vector<uint16_t> data_;
    };

    [[nodiscard]] int randomDirection();
    void resetRouteData();
    void initializeBorders();
    void rebuildWholeTexture();
    void refreshPixels(const DirtyRegion& region);
    void updatePixel(uint32_t x, uint32_t y, uint16_t state);
    void mergeDirtyRegion(const DirtyRegion& region);
    void markDirtyCell(DirtyRegion& region, uint32_t x, uint32_t y);
    void markWholeGridDirty();
    void gatherNeighbours(std::array<uint16_t, 8>& out, const Matrix& matrix, uint32_t x, uint32_t y) const;
    void physarumTransitionConditions(
        uint32_t x,
        uint32_t y,
        uint16_t currentCell,
        const std::array<uint16_t, 8>& neighboursData,
        const std::array<uint16_t, 8>& neighboursDirections,
        DirtyRegion& dirtyRegion);
    [[nodiscard]] static bool hasNeighbourState(
        const std::array<uint16_t, 8>& neighboursData,
        int direction,
        uint16_t state);
    [[nodiscard]] static bool isOnMooreOffset(const std::array<uint16_t, 8>& neighboursDirections);
    static void findStates(const std::array<uint16_t, 8>& neighboursData, std::array<bool, 9>& statesFound);
    static void applyCornerWalls(std::array<uint16_t, 8>& neighboursData);

private:
    GridSize gridSize_{};
    Palette palette_ = defaultPalette();
    Matrix physarumMatrix_{};
    Matrix auxMatrix_{};
    Matrix memoryMatrix_{};
    std::vector<uint8_t> rgbaPixels_{};
    DirtyRegion dirtyRegion_{};
    bool forceFullUpload_ = false;

    int nutrientNotFounded_ = 0;
    int nutrientFounded_ = 0;
    int physarumActualCells_ = 0;
    int physarumLastCells_ = 0;
    int minimumPhysarumCells_ = 0;
    int minimumCheck_ = 0;
    bool allNutrientsFounded_ = false;
    bool routed_ = false;
    std::array<int, 9> statesDensity_{};
};
