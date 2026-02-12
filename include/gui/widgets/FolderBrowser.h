#pragma once

#include <filesystem>
#include <string>
#include <functional>

#include "imgui.h"
#include "imfilebrowser.h"

namespace fs = std::filesystem;

using FolderSelectedCallback = std::function<void(const fs::path&)>;
using FolderCancelledCallback = std::function<void()>;

class FolderBrowser {
public:
    FolderBrowser();

    // Window Lifecycle & Rendering
    void Open();
    void Close();
    void Draw();

    // Event callbacks
    void SetOnFolderSelected(const FolderSelectedCallback &callback);
    void SetOnCancelled(const FolderCancelledCallback &callback);

    // Configuration & Settings
    void SetTitle(const std::string& title);
    void SetInitialPath(const fs::path& path);
    void SetWindowSize(int width, int height);

    // State queries
    bool IsOpen() const noexcept;
    bool HasSelected() const noexcept;

    // Selection data
    fs::path GetSelectedPath() const;
    void ClearSelection();
private:
    // Window Configuration
    static void DrawDimBackground();

    // ImGui file dialog
    ImGui::FileBrowser m_fileDialog;

    // Event callbacks
    FolderSelectedCallback m_onSelected;
    FolderCancelledCallback m_onCancelled;

    // Configuration
    std::string m_title;
    ImVec2 m_windowSize;

    // State
    bool m_shouldDraw;
    bool m_selectionSuccess;
    fs::path m_currentPath;
};