#pragma once

#include "data/VideoInfo.h"
#include "core/recording/RecordingManager.h"
#include "gui/core/BaseScreen.h"
#include "gui/widgets/FolderBrowser.h"
#include "gui/screens/main/TopBar.h"

#include <vector>
#include <memory>
#include <filesystem>


struct VideoInfo;

class VideoLibrary;
class WelcomeState;
class LoadingState;
class VideoListState;
class EmptyFolderState;

enum class MainScreenState {
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
    void RefreshLibrarySilent();

    // State management
    void ChangeState(const MainScreenState state) { m_currentState = state; }
    const char* GetCurrentWindowName() const;

    // Data access
    std::vector<VideoInfo>& GetCurrentVideos() { return m_currentVideos; }
    void SetCurrentVideos(const std::vector<VideoInfo>& videos) { m_currentVideos = videos; }

    const std::filesystem::path& GetCurrentFolder() const { return m_currentFolder; }

    // Component access
    FolderBrowser& GetFolderBrowser() { return m_folderBrowser; }
    RecordingManager* GetRecordingManagerPtr() const { return m_recordingManager.get(); }

    bool IsRecordingActive() const {return m_recordingManager && m_recordingManager->IsRecording();}
private:
    // State
    MainScreenState m_currentState = MainScreenState::WELCOME;

    // UI States
    std::unique_ptr<WelcomeState> m_welcomeState;
    std::unique_ptr<LoadingState> m_loadingState;
    std::unique_ptr<VideoListState> m_videoListState;
    std::unique_ptr<EmptyFolderState> m_emptyFolderState;

    // UI Components
    std::unique_ptr<TopBar> m_topBar;
    std::unique_ptr<RecordingManager> m_recordingManager;

    FolderBrowser m_folderBrowser;

    // Data
    std::vector<VideoInfo> m_currentVideos;
    std::filesystem::path m_currentFolder;
};