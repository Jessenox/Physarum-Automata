#pragma once

#include <filesystem>
#include <optional>

struct ImageFileDialogResult {
    bool available = false;
    std::optional<std::filesystem::path> path;
};

struct SaveFileDialogResult {
    bool available = false;
    std::optional<std::filesystem::path> path;
};

ImageFileDialogResult pickImageFile();
SaveFileDialogResult pickSaveSvgFile();
SaveFileDialogResult pickSavePngFile();
