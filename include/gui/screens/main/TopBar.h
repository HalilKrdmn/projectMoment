#pragma once

#include <string>
#include <functional>

class MainScreen;
class RecordingManager;

struct StorageInfo {
    size_t totalVideos = 0;
    float usedSpaceGB = 0.0f;
    float totalSpaceGB = 0.0f;
    float freeSpaceGB = 0.0f;
};

class TopBar {
public:
    void Draw(MainScreen* parent, const RecordingManager* recordingManager = nullptr) const;

    void SetOnSettingsClicked(const std::function<void()> &callback) {
        m_onSettingsClicked = callback;
    }

    void SetOnRecordClicked(const std::function<void()> &callback) {
        m_onRecordClicked = callback;
    }

private:
    void DrawRecordingButton(const RecordingManager *recordingManager) const;
    static void DrawStorageInfo(const StorageInfo& info);

    static StorageInfo CalculateStorageInfo(const std::string& libraryPath, size_t videoCount);

    std::function<void()> m_onSettingsClicked;
    std::function<void()> m_onRecordClicked;
};