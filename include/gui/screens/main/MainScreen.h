#pragma once

#include "gui/core/BaseScreen.h"
#include "gui/screens/main/states/WelcomeState.h"
#include "gui/screens/main/states/LoadingState.h"
#include "gui/screens/main/states/VideoListState.h"
#include "gui/screens/main/states/EmptyFolderState.h"
#include "gui/widgets/FolderBrowser.h"
#include "core/library/VideoLibrary.h"

#include <functional>
#include <string>
#include <vector>
#include <filesystem>

 // ──────────────────────────────────────────────────────────────────────────
struct StorageInfo {
    size_t totalVideos  = 0;
    float  totalSpaceGB = 0.0f;
    float  usedSpaceGB  = 0.0f;
    float  freeSpaceGB  = 0.0f;
};

enum class MainScreenState {
    WELCOME,
    LOADING,
    VIDEO_LIST,
    EMPTY_FOLDER
};
// ──────────────────────────────────────────────────────────────────────────

class MainScreen : public BaseScreen {
public:
    explicit MainScreen(MainWindow* manager);

    void Draw() override;

    // ── Public API ────────────────────────────────────────────────────────────
    std::vector<VideoInfo>& GetCurrentVideos() { return m_currentVideos; }
    void SetCurrentVideos(const std::vector<VideoInfo>& videos) { m_currentVideos = videos; }

    FolderBrowser& GetFolderBrowser() { return m_folderBrowser; }
    std::filesystem::path GetCurrentFolder() const;
    void DetermineInitialState();
    void SetOnSettingsClicked(std::function<void()> cb) { m_onSettingsClicked = std::move(cb); }

private:
    // ── State ─────────────────────────────────────────────────────────────────
    MainScreenState m_currentState = MainScreenState::WELCOME;

    std::unique_ptr<WelcomeState>     m_welcomeState;
    std::unique_ptr<LoadingState>     m_loadingState;
    std::unique_ptr<VideoListState>   m_videoListState;
    std::unique_ptr<EmptyFolderState> m_emptyFolderState;

    std::vector<VideoInfo> m_currentVideos;
    FolderBrowser          m_folderBrowser;

    // ── Callbacks ─────────────────────────────────────────────────────────────
    std::function<void()> m_onSettingsClicked;

    // ── Library helpers ───────────────────────────────────────────────────────
    bool ValidateLibraryPath();
    void StartLibraryLoad();
    void RefreshLibrarySilent();
    void ChangeState(MainScreenState state) { m_currentState = state; }
    const char* GetCurrentWindowName() const;

    // ── TopBar ─────────────────────────────────────────────────────────────────
    void DrawTopBar();
    void DrawRecordToggleButton();
    void DrawClipSaveButton();
    void DrawStorageInfo(const StorageInfo& info);
    static StorageInfo CalculateStorageInfo(const std::string& libraryPath, size_t videoCount);
};