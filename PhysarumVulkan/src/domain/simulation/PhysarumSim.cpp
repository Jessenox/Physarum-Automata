#include "domain/simulation/PhysarumSim.h"

#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>

#ifndef PHYSARUM_VULKAN_HAS_OPENCV
#define PHYSARUM_VULKAN_HAS_OPENCV 0
#endif

#if PHYSARUM_VULKAN_HAS_OPENCV
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#endif

namespace {

constexpr PhysarumSim::Palette kDefaultPhysarumColors{{
    PhysarumSim::Rgba{26, 26, 112, 255},
    PhysarumSim::Rgba{122, 105, 237, 255},
    PhysarumSim::Rgba{255, 0, 0, 255},
    PhysarumSim::Rgba{0, 0, 0, 255},
    PhysarumSim::Rgba{255, 224, 54, 255},
    PhysarumSim::Rgba{0, 128, 0, 255},
    PhysarumSim::Rgba{250, 232, 181, 255},
    PhysarumSim::Rgba{46, 79, 79, 255},
    PhysarumSim::Rgba{133, 186, 102, 255},
}};

// Order preserved from the original SFML implementation.
constexpr std::array<std::pair<int, int>, 8> kMooreOffsets{{
    {-1, 0},
    {-1, 1},
    {0, 1},
    {1, 1},
    {1, 0},
    {1, -1},
    {0, -1},
    {-1, -1},
}};

#if PHYSARUM_VULKAN_HAS_OPENCV
int denoiseKernelSize(const GridSize size) {
    const uint32_t minDimension = std::min(size.w, size.h);
    if (minDimension >= 1500U) {
        return 5;
    }
    return 3;
}

int minimumObstacleArea(const GridSize size) {
    const uint64_t totalArea = static_cast<uint64_t>(size.w) * static_cast<uint64_t>(size.h);
    return std::max<int>(12, static_cast<int>(totalArea / 40'000ULL));
}

int maximumHoleArea(const GridSize size) {
    const uint64_t totalArea = static_cast<uint64_t>(size.w) * static_cast<uint64_t>(size.h);
    return std::max<int>(24, static_cast<int>(totalArea / 6'000ULL));
}

void removeSmallForegroundComponents(cv::Mat& binaryMask, const int minArea) {
    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int componentCount =
        cv::connectedComponentsWithStats(binaryMask, labels, stats, centroids, 8, CV_32S);

    cv::Mat filteredMask = cv::Mat::zeros(binaryMask.size(), CV_8UC1);
    for (int label = 1; label < componentCount; ++label) {
        const int componentArea = stats.at<int>(label, cv::CC_STAT_AREA);
        if (componentArea < minArea) {
            continue;
        }
        filteredMask.setTo(255, labels == label);
    }

    binaryMask = filteredMask;
}

void fillSmallInteriorHoles(cv::Mat& binaryMask, const int maxArea) {
    cv::Mat invertedMask;
    cv::bitwise_not(binaryMask, invertedMask);

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int componentCount =
        cv::connectedComponentsWithStats(invertedMask, labels, stats, centroids, 8, CV_32S);

    for (int label = 1; label < componentCount; ++label) {
        const int left = stats.at<int>(label, cv::CC_STAT_LEFT);
        const int top = stats.at<int>(label, cv::CC_STAT_TOP);
        const int width = stats.at<int>(label, cv::CC_STAT_WIDTH);
        const int height = stats.at<int>(label, cv::CC_STAT_HEIGHT);
        const int componentArea = stats.at<int>(label, cv::CC_STAT_AREA);

        const bool touchesBorder =
            left == 0 ||
            top == 0 ||
            (left + width) >= binaryMask.cols ||
            (top + height) >= binaryMask.rows;
        if (touchesBorder || componentArea > maxArea) {
            continue;
        }

        binaryMask.setTo(255, labels == label);
    }
}

cv::Mat obstacleMaskFromGrayImage(const cv::Mat& grayImage, const GridSize size) {
    cv::Mat blurredImage;
    cv::GaussianBlur(grayImage, blurredImage, cv::Size(5, 5), 0.0, 0.0);

    cv::Mat obstacleMask;
    cv::threshold(
        blurredImage,
        obstacleMask,
        0.0,
        255.0,
        cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    const int kernelSize = denoiseKernelSize(size);
    const cv::Mat kernel = cv::getStructuringElement(
        cv::MORPH_ELLIPSE,
        cv::Size(kernelSize, kernelSize));
    cv::morphologyEx(obstacleMask, obstacleMask, cv::MORPH_CLOSE, kernel);

    removeSmallForegroundComponents(obstacleMask, minimumObstacleArea(size));
    fillSmallInteriorHoles(obstacleMask, maximumHoleArea(size));
    cv::morphologyEx(obstacleMask, obstacleMask, cv::MORPH_CLOSE, kernel);

    return obstacleMask;
}
#endif

}  // namespace

void PhysarumSim::Matrix::resize(const GridSize size) {
    size_ = size;
    data_.assign(static_cast<std::size_t>(size.w) * static_cast<std::size_t>(size.h), 0U);
}

std::size_t PhysarumSim::Matrix::indexOf(const uint32_t x, const uint32_t y) const {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(size_.w) + static_cast<std::size_t>(x);
}

uint16_t PhysarumSim::Matrix::get(const uint32_t x, const uint32_t y) const {
    return data_[indexOf(x, y)];
}

uint16_t& PhysarumSim::Matrix::at(const uint32_t x, const uint32_t y) {
    return data_[indexOf(x, y)];
}

void PhysarumSim::Matrix::set(const uint32_t x, const uint32_t y, const uint16_t value) {
    at(x, y) = value;
}

PhysarumSim::PhysarumSim(const GridSize size) {
    resizeGrid(size);
}

void PhysarumSim::resizeGrid(const GridSize newSize) {
    if (newSize.w == 0 || newSize.h == 0) {
        throw std::runtime_error("Grid size must be greater than zero.");
    }

    gridSize_ = newSize;
    physarumMatrix_.resize(gridSize_);
    auxMatrix_.resize(gridSize_);
    memoryMatrix_.resize(gridSize_);
    rgbaPixels_.assign(
        static_cast<std::size_t>(gridSize_.w) * static_cast<std::size_t>(gridSize_.h) * 4U,
        0U);

    resetRouteData();
    initializeBorders();
    rebuildWholeTexture();
    markWholeGridDirty();
}

void PhysarumSim::setCellState(const uint32_t x, const uint32_t y, uint16_t value) {
    if (x >= gridSize_.w || y >= gridSize_.h) {
        return;
    }

    value = std::min<uint16_t>(value, 8U);
    if (physarumMatrix_.get(x, y) == value) {
        return;
    }

    physarumMatrix_.set(x, y, value);
    memoryMatrix_.set(x, y, 0U);
    updatePixel(x, y, value);

    DirtyRegion dirty{};
    markDirtyCell(dirty, x, y);
    mergeDirtyRegion(dirty);

    routed_ = false;
    allNutrientsFounded_ = false;
}

void PhysarumSim::overwriteCellState(const uint32_t x, const uint32_t y, uint16_t value) {
    if (x >= gridSize_.w || y >= gridSize_.h) {
        return;
    }

    value = std::min<uint16_t>(value, 8U);
    physarumMatrix_.set(x, y, value);
    memoryMatrix_.set(x, y, 0U);
    updatePixel(x, y, value);

    DirtyRegion dirty{};
    markDirtyCell(dirty, x, y);
    mergeDirtyRegion(dirty);

    routed_ = false;
    allNutrientsFounded_ = false;
}

void PhysarumSim::setPaletteColor(const uint8_t state, const Rgba color) {
    const std::size_t stateIndex = std::min<std::size_t>(state, palette_.size() - 1U);
    if (palette_[stateIndex] == color) {
        return;
    }

    palette_[stateIndex] = color;
    rebuildWholeTexture();
}

void PhysarumSim::loadMapFromImage(const std::filesystem::path& imagePath) {
#if !PHYSARUM_VULKAN_HAS_OPENCV
    (void)imagePath;
    throw std::runtime_error("OpenCV support is not enabled in this build.");
#else
    const cv::Mat actualImage = cv::imread(imagePath.string(), cv::IMREAD_COLOR);
    if (actualImage.empty()) {
        throw std::runtime_error("Unable to open image: " + imagePath.string());
    }

    cv::Mat resizedImage;
    cv::resize(
        actualImage,
        resizedImage,
        cv::Size(static_cast<int>(gridSize_.w), static_cast<int>(gridSize_.h)),
        0.0,
        0.0,
        cv::INTER_LINEAR);

    cv::Mat grayImage;
    cv::cvtColor(resizedImage, grayImage, cv::COLOR_BGR2GRAY);
    const cv::Mat obstacleMask = obstacleMaskFromGrayImage(grayImage, gridSize_);

    physarumMatrix_.resize(gridSize_);
    auxMatrix_.resize(gridSize_);
    memoryMatrix_.resize(gridSize_);

    for (uint32_t y = 0; y < gridSize_.h; ++y) {
        for (uint32_t x = 0; x < gridSize_.w; ++x) {
            const bool isObstacle =
                obstacleMask.at<uint8_t>(static_cast<int>(y), static_cast<int>(x)) > 0;
            uint16_t cellState = isObstacle ? 2U : 0U;
            if (x == 0 || y == 0 || x == gridSize_.w - 1U || y == gridSize_.h - 1U) {
                cellState = 2U;
            }
            physarumMatrix_.set(x, y, cellState);
        }
    }

    resetRouteData();
    rebuildWholeTexture();
    markWholeGridDirty();
#endif
}

void PhysarumSim::evaluatePhysarum() {
    statesDensity_.fill(0);
    auxMatrix_ = physarumMatrix_;

    DirtyRegion stepDirty{};
    std::array<uint16_t, 8> neighboursData{};
    std::array<uint16_t, 8> neighboursDirections{};

    for (uint32_t y = 0; y < gridSize_.h; ++y) {
        for (uint32_t x = 0; x < gridSize_.w; ++x) {
            gatherNeighbours(neighboursData, physarumMatrix_, x, y);
            gatherNeighbours(neighboursDirections, memoryMatrix_, x, y);
            physarumTransitionConditions(
                x,
                y,
                physarumMatrix_.get(x, y),
                neighboursData,
                neighboursDirections,
                stepDirty);
        }
    }

    std::swap(physarumMatrix_, auxMatrix_);

    if (stepDirty.valid) {
        refreshPixels(stepDirty);
        mergeDirtyRegion(stepDirty);
    }

    if (nutrientNotFounded_ == 0 && nutrientFounded_ > 0) {
        allNutrientsFounded_ = true;

        if (physarumActualCells_ < physarumLastCells_) {
            minimumPhysarumCells_ = physarumActualCells_;
        }

        minimumCheck_ = (minimumPhysarumCells_ == physarumLastCells_) ? (minimumCheck_ + 1) : 0;
        if (minimumCheck_ > 10) {
            routed_ = true;
        }
    }

    physarumLastCells_ = physarumActualCells_;
    nutrientNotFounded_ = 0;
    nutrientFounded_ = 0;
    physarumActualCells_ = 0;
}

void PhysarumSim::clearDirtyTracking() {
    dirtyRegion_ = {};
    forceFullUpload_ = false;
}

const PhysarumSim::Palette& PhysarumSim::defaultPalette() {
    return kDefaultPhysarumColors;
}

PhysarumSim::Rgba PhysarumSim::colorForState(const uint8_t state) const {
    return palette_.at(std::min<std::size_t>(state, palette_.size() - 1U));
}

int PhysarumSim::randomDirection() {
    static thread_local std::mt19937 generator(std::random_device{}());
    static thread_local std::uniform_int_distribution<int> distribution(0, 7);
    return distribution(generator);
}

void PhysarumSim::resetRouteData() {
    nutrientNotFounded_ = 0;
    nutrientFounded_ = 0;
    physarumActualCells_ = 0;
    physarumLastCells_ = 0;
    minimumPhysarumCells_ = 0;
    minimumCheck_ = 0;
    allNutrientsFounded_ = false;
    routed_ = false;
    statesDensity_.fill(0);
}

void PhysarumSim::initializeBorders() {
    if (gridSize_.w == 0 || gridSize_.h == 0) {
        return;
    }

    for (uint32_t x = 0; x < gridSize_.w; ++x) {
        physarumMatrix_.set(x, 0, 2);
        physarumMatrix_.set(x, gridSize_.h - 1U, 2);
    }

    for (uint32_t y = 0; y < gridSize_.h; ++y) {
        physarumMatrix_.set(0, y, 2);
        physarumMatrix_.set(gridSize_.w - 1U, y, 2);
    }
}

void PhysarumSim::rebuildWholeTexture() {
    DirtyRegion wholeGrid{};
    if (gridSize_.w == 0 || gridSize_.h == 0) {
        return;
    }

    wholeGrid.valid = true;
    wholeGrid.minX = 0;
    wholeGrid.minY = 0;
    wholeGrid.maxX = gridSize_.w - 1U;
    wholeGrid.maxY = gridSize_.h - 1U;
    refreshPixels(wholeGrid);
}

void PhysarumSim::refreshPixels(const DirtyRegion& region) {
    if (!region.valid) {
        return;
    }

    for (uint32_t y = region.minY; y <= region.maxY; ++y) {
        for (uint32_t x = region.minX; x <= region.maxX; ++x) {
            updatePixel(x, y, physarumMatrix_.get(x, y));
        }
    }
}

void PhysarumSim::updatePixel(const uint32_t x, const uint32_t y, const uint16_t state) {
    const Rgba color = colorForState(static_cast<uint8_t>(state));
    const std::size_t baseIndex =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(gridSize_.w) + static_cast<std::size_t>(x)) * 4U;

    rgbaPixels_.at(baseIndex + 0U) = color[0];
    rgbaPixels_.at(baseIndex + 1U) = color[1];
    rgbaPixels_.at(baseIndex + 2U) = color[2];
    rgbaPixels_.at(baseIndex + 3U) = color[3];
}

void PhysarumSim::mergeDirtyRegion(const DirtyRegion& region) {
    if (!region.valid) {
        return;
    }

    if (!dirtyRegion_.valid) {
        dirtyRegion_ = region;
        return;
    }

    dirtyRegion_.minX = std::min(dirtyRegion_.minX, region.minX);
    dirtyRegion_.minY = std::min(dirtyRegion_.minY, region.minY);
    dirtyRegion_.maxX = std::max(dirtyRegion_.maxX, region.maxX);
    dirtyRegion_.maxY = std::max(dirtyRegion_.maxY, region.maxY);
}

void PhysarumSim::markDirtyCell(DirtyRegion& region, const uint32_t x, const uint32_t y) {
    if (!region.valid) {
        region.valid = true;
        region.minX = x;
        region.maxX = x;
        region.minY = y;
        region.maxY = y;
        return;
    }

    region.minX = std::min(region.minX, x);
    region.minY = std::min(region.minY, y);
    region.maxX = std::max(region.maxX, x);
    region.maxY = std::max(region.maxY, y);
}

void PhysarumSim::markWholeGridDirty() {
    dirtyRegion_.valid = true;
    dirtyRegion_.minX = 0;
    dirtyRegion_.minY = 0;
    dirtyRegion_.maxX = gridSize_.w - 1U;
    dirtyRegion_.maxY = gridSize_.h - 1U;
    forceFullUpload_ = true;
}

uint32_t PhysarumSim::combinedState(const uint32_t x, const uint32_t y) const {
    return static_cast<uint32_t>(physarumMatrix_.get(x, y)) +
           static_cast<uint32_t>(memoryMatrix_.get(x, y)) * 9U;
}

void PhysarumSim::packCombinedStates(void* destination) const {
    auto* packedStates = static_cast<uint32_t*>(destination);
    for (uint32_t y = 0; y < gridSize_.h; ++y) {
        for (uint32_t x = 0; x < gridSize_.w; ++x) {
            packedStates[static_cast<std::size_t>(y) * static_cast<std::size_t>(gridSize_.w) + x] =
                combinedState(x, y);
        }
    }
}

void PhysarumSim::packCombinedStatesRegion(const DirtyRegion& region, void* destination) const {
    if (!region.valid) {
        return;
    }

    auto* packedStates = static_cast<uint32_t*>(destination);
    for (uint32_t y = region.minY; y <= region.maxY; ++y) {
        const std::size_t destinationRow =
            static_cast<std::size_t>(y - region.minY) * static_cast<std::size_t>(region.width());
        for (uint32_t x = region.minX; x <= region.maxX; ++x) {
            packedStates[destinationRow + static_cast<std::size_t>(x - region.minX)] = combinedState(x, y);
        }
    }
}

void PhysarumSim::gatherNeighbours(
    std::array<uint16_t, 8>& out,
    const Matrix& matrix,
    const uint32_t x,
    const uint32_t y) const {
    for (std::size_t index = 0; index < kMooreOffsets.size(); ++index) {
        const auto& [deltaX, deltaY] = kMooreOffsets[index];
        const uint32_t sampleX = static_cast<uint32_t>(
            std::clamp(static_cast<int>(x) + deltaX, 0, static_cast<int>(gridSize_.w) - 1));
        const uint32_t sampleY = static_cast<uint32_t>(
            std::clamp(static_cast<int>(y) + deltaY, 0, static_cast<int>(gridSize_.h) - 1));
        out[index] = matrix.get(sampleX, sampleY);
    }
}

void PhysarumSim::physarumTransitionConditions(
    const uint32_t x,
    const uint32_t y,
    const uint16_t currentCell,
    const std::array<uint16_t, 8>& neighboursDataInput,
    const std::array<uint16_t, 8>& neighboursDirections,
    DirtyRegion& dirtyRegion) {
    std::array<uint16_t, 8> neighboursData = neighboursDataInput;
    applyCornerWalls(neighboursData);

    const int currentDirection = randomDirection();
    std::array<bool, 9> statesFound{};
    findStates(neighboursData, statesFound);

    const auto writeAux = [&](const uint16_t newState) {
        if (auxMatrix_.get(x, y) != newState) {
            auxMatrix_.set(x, y, newState);
            markDirtyCell(dirtyRegion, x, y);
        }
    };

    switch (currentCell) {
        case 0:
            if ((hasNeighbourState(neighboursData, currentDirection, 3) ||
                 hasNeighbourState(neighboursData, currentDirection, 4) ||
                 hasNeighbourState(neighboursData, currentDirection, 6)) &&
                memoryMatrix_.get(x, y) == 0) {
                writeAux(7);
            }
            ++statesDensity_[0];
            break;
        case 1:
            if (statesFound[5] || statesFound[6]) {
                writeAux(6);
            }
            ++nutrientNotFounded_;
            ++statesDensity_[1];
            break;
        case 2:
            ++statesDensity_[2];
            break;
        case 3:
            ++statesDensity_[3];
            break;
        case 4:
            if ((hasNeighbourState(neighboursData, currentDirection, 3) ||
                 hasNeighbourState(neighboursData, currentDirection, 5) ||
                 hasNeighbourState(neighboursData, currentDirection, 6)) &&
                memoryMatrix_.get(x, y) == 0 &&
                !statesFound[0] &&
                !statesFound[7]) {
                writeAux(5);
                memoryMatrix_.set(x, y, static_cast<uint16_t>(currentDirection + 1));
            }
            ++statesDensity_[4];
            break;
        case 5:
            if (!isOnMooreOffset(neighboursDirections) &&
                !isOnMooreOffset(neighboursDirections) &&
                !statesFound[1] &&
                !statesFound[3] &&
                !statesFound[4] &&
                !statesFound[6]) {
                writeAux(0);
                memoryMatrix_.set(x, y, 0);
            } else {
                writeAux(8);
            }
            ++physarumActualCells_;
            ++statesDensity_[5];
            break;
        case 6:
            ++nutrientFounded_;
            ++statesDensity_[6];
            break;
        case 7:
            if (statesFound[3] || statesFound[4] || statesFound[6]) {
                writeAux(4);
            }
            ++statesDensity_[7];
            break;
        case 8:
            writeAux(5);
            ++physarumActualCells_;
            ++statesDensity_[8];
            break;
        default:
            break;
    }
}

bool PhysarumSim::hasNeighbourState(
    const std::array<uint16_t, 8>& neighboursData,
    const int direction,
    const uint16_t state) {
    return neighboursData.at(static_cast<std::size_t>(direction)) == state;
}

bool PhysarumSim::isOnMooreOffset(const std::array<uint16_t, 8>& neighboursDirections) {
    for (std::size_t index = 4, expected = 0; expected < neighboursDirections.size(); ++index, ++expected) {
        if (index > 7) {
            index = 0;
        }
        if (neighboursDirections[index] > 0 &&
            static_cast<std::size_t>(neighboursDirections[index] - 1U) == expected) {
            return true;
        }
    }
    return false;
}

void PhysarumSim::findStates(
    const std::array<uint16_t, 8>& neighboursData,
    std::array<bool, 9>& statesFound) {
    statesFound.fill(false);
    for (const uint16_t value : neighboursData) {
        if (value < statesFound.size()) {
            statesFound[value] = true;
        }
    }
}

void PhysarumSim::applyCornerWalls(std::array<uint16_t, 8>& neighboursData) {
    if (neighboursData[0] == 2 && neighboursData[2] == 2) {
        neighboursData[1] = 2;
    }
    if (neighboursData[0] == 2 && neighboursData[6] == 2) {
        neighboursData[7] = 2;
    }
    if (neighboursData[6] == 2 && neighboursData[4] == 2) {
        neighboursData[5] = 2;
    }
    if (neighboursData[2] == 2 && neighboursData[4] == 2) {
        neighboursData[3] = 2;
    }
}
