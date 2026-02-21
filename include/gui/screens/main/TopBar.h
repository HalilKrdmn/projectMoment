#pragma once

#include <functional>
#include <string>

class MainScreen;
class RecordingManager;

struct StorageInfo {
    size_t totalVideos  = 0;
    float  totalSpaceGB = 0.0f;
    float  freeSpaceGB  = 0.0f;
    float  usedSpaceGB  = 0.0f;
};

class TopBar {
public:
    void Draw(MainScreen* parent, RecordingManager* recordingManager);

    void DrawRecordToggleButton(RecordingManager *recordingManager);

    void SetOnSettingsClicked(std::function<void()> cb) { m_onSettingsClicked = std::move(cb); }

private:
    void DrawClipSaveButton();
    static void DrawStorageInfo(const StorageInfo& info);
    static StorageInfo CalculateStorageInfo(const std::string& libraryPath, size_t videoCount);

    std::function<void()> m_onSettingsClicked;
};