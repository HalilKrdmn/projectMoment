#pragma once

#include <atomic>

#include "data/VideoInfo.h"
#include "core/library/VideoLibrary.h"
#include "gui/core/BaseScreen.h"
#include "gui/widgets/FolderBrowser.h"

#include <vector>
#include <chrono>
#include <memory>
#include <filesystem>
#include <thread>

#include "core/import/VideoImportService.h"

class VideoLibrary;
struct VideoInfo;

class WelcomeState;
class LoadingState;
class VideoListState;
class EmptyFolderState;

enum class ContentState {
    LOADING,
    WELCOME,
    EMPTY_FOLDER,
    VIDEO_LIST
};

class MainScreen : public BaseScreen {
public:
    explicit MainScreen(MainWindow* manager);

    // Lifecycle methods
    void DetermineInitialState();
    void Draw() override;

    // Public API
    static bool ValidateLibraryPath();
    void StartLibraryLoad();

    // State management
    void ChangeState(const ContentState state) { m_currentState = state; }
    const char* GetCurrentWindowName() const;

    // Data access
    std::vector<VideoInfo>& GetCurrentVideos() { return m_currentVideos; }
    void SetCurrentVideos(const std::vector<VideoInfo>& videos) { m_currentVideos = videos; }

    const std::filesystem::path& GetCurrentFolder() const { return m_currentFolder; }

    // Component access
    FolderBrowser& GetFolderBrowser() { return m_folderBrowser; }
private:
    // State
    ContentState m_currentState = ContentState::WELCOME;

    // UI States
    std::unique_ptr<WelcomeState> m_welcomeState;
    std::unique_ptr<LoadingState> m_loadingState;
    std::unique_ptr<VideoListState> m_videoListState;
    std::unique_ptr<EmptyFolderState> m_emptyFolderState;

    // UI Components
    FolderBrowser m_folderBrowser;

    // Data
    std::vector<VideoInfo> m_currentVideos;
    std::filesystem::path m_currentFolder;
};