#include "FileDialog.h"

#include <array>
#include <cstdio>
#include <string>

namespace {

std::string trimTrailingWhitespace(std::string value) {
    while (!value.empty() &&
           (value.back() == '\n' || value.back() == '\r' || value.back() == '\t' || value.back() == ' ')) {
        value.pop_back();
    }
    return value;
}

std::optional<std::string> runDialogCommand(const char* command) {
    FILE* pipe = popen(command, "r");
    if (pipe == nullptr) {
        return std::nullopt;
    }

    std::string output;
    std::array<char, 256> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

    const int exitCode = pclose(pipe);
    output = trimTrailingWhitespace(output);
    if (exitCode != 0 || output.empty()) {
        return std::nullopt;
    }

    return output;
}

}  // namespace

ImageFileDialogResult pickImageFile() {
    ImageFileDialogResult result{};

#if defined(__linux__)
    struct DialogCommand {
        const char* available;
        const char* run;
    };

    static constexpr std::array<DialogCommand, 3> kCommands{{
        {
            "command -v zenity >/dev/null 2>&1 && printf ok",
            "zenity --file-selection --title='Selecciona un mapa' "
            "--file-filter='Imagenes | *.png *.jpg *.jpeg *.bmp *.tif *.tiff' 2>/dev/null"
        },
        {
            "command -v kdialog >/dev/null 2>&1 && printf ok",
            "kdialog --getopenfilename . '*.png *.jpg *.jpeg *.bmp *.tif *.tiff|Imagenes' 2>/dev/null"
        },
        {
            "command -v python3 >/dev/null 2>&1 && printf ok",
            "python3 -c \"import tkinter as tk; "
            "from tkinter import filedialog; "
            "root = tk.Tk(); "
            "root.withdraw(); "
            "root.attributes('-topmost', True); "
            "path = filedialog.askopenfilename("
            "title='Selecciona un mapa', "
            "filetypes=[('Imagenes', '*.png *.jpg *.jpeg *.bmp *.tif *.tiff')]); "
            "print(path)\" 2>/dev/null"
        }
    }};

    for (const DialogCommand& command : kCommands) {
        if (!runDialogCommand(command.available).has_value()) {
            continue;
        }

        result.available = true;
        const std::optional<std::string> selectedFile = runDialogCommand(command.run);
        if (!selectedFile.has_value()) {
            continue;
        }

        std::filesystem::path path(selectedFile.value());
        if (std::filesystem::exists(path)) {
            result.path = path;
            return result;
        }
    }
#endif

    return result;
}

SaveFileDialogResult pickSaveSvgFile() {
    SaveFileDialogResult result{};

#if defined(__linux__)
    struct DialogCommand {
        const char* available;
        const char* run;
    };

    static constexpr std::array<DialogCommand, 3> kCommands{{
        {
            "command -v zenity >/dev/null 2>&1 && printf ok",
            "zenity --file-selection --save --confirm-overwrite "
            "--title='Exportar atractor SVG' "
            "--filename='attractor.svg' "
            "--file-filter='SVG | *.svg' 2>/dev/null"
        },
        {
            "command -v kdialog >/dev/null 2>&1 && printf ok",
            "kdialog --getsavefilename . 'attractor.svg|SVG (*.svg)' 2>/dev/null"
        },
        {
            "command -v python3 >/dev/null 2>&1 && printf ok",
            "python3 -c \"import tkinter as tk; "
            "from tkinter import filedialog; "
            "root = tk.Tk(); "
            "root.withdraw(); "
            "root.attributes('-topmost', True); "
            "path = filedialog.asksaveasfilename("
            "title='Exportar atractor SVG', "
            "defaultextension='.svg', "
            "initialfile='attractor.svg', "
            "filetypes=[('SVG', '*.svg')]); "
            "print(path)\" 2>/dev/null"
        }
    }};

    for (const DialogCommand& command : kCommands) {
        if (!runDialogCommand(command.available).has_value()) {
            continue;
        }

        result.available = true;
        const std::optional<std::string> selectedFile = runDialogCommand(command.run);
        if (!selectedFile.has_value()) {
            continue;
        }

        std::filesystem::path path(selectedFile.value());
        if (path.extension().empty()) {
            path.replace_extension(".svg");
        }
        result.path = path;
        return result;
    }
#endif

    return result;
}

SaveFileDialogResult pickSavePngFile() {
    SaveFileDialogResult result{};

#if defined(__linux__)
    struct DialogCommand {
        const char* available;
        const char* run;
    };

    static constexpr std::array<DialogCommand, 3> kCommands{{
        {
            "command -v zenity >/dev/null 2>&1 && printf ok",
            "zenity --file-selection --save --confirm-overwrite "
            "--title='Exportar atractor PNG' "
            "--filename='attractor.png' "
            "--file-filter='PNG | *.png' 2>/dev/null"
        },
        {
            "command -v kdialog >/dev/null 2>&1 && printf ok",
            "kdialog --getsavefilename . 'attractor.png|PNG (*.png)' 2>/dev/null"
        },
        {
            "command -v python3 >/dev/null 2>&1 && printf ok",
            "python3 -c \"import tkinter as tk; "
            "from tkinter import filedialog; "
            "root = tk.Tk(); "
            "root.withdraw(); "
            "root.attributes('-topmost', True); "
            "path = filedialog.asksaveasfilename("
            "title='Exportar atractor PNG', "
            "defaultextension='.png', "
            "initialfile='attractor.png', "
            "filetypes=[('PNG', '*.png')]); "
            "print(path)\" 2>/dev/null"
        }
    }};

    for (const DialogCommand& command : kCommands) {
        if (!runDialogCommand(command.available).has_value()) {
            continue;
        }

        result.available = true;
        const std::optional<std::string> selectedFile = runDialogCommand(command.run);
        if (!selectedFile.has_value()) {
            continue;
        }

        std::filesystem::path path(selectedFile.value());
        if (path.extension().empty()) {
            path.replace_extension(".png");
        }
        result.path = path;
        return result;
    }
#endif

    return result;
}
