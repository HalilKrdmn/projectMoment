#pragma once

// #include "core/recording/OBSController.h"
#include "core/recording/NativeRecorder.h"


enum class RecordingMode {
    OBS,
    NATIVE
};


class RecordingManager {
public:
    RecordingManager();
    ~RecordingManager();

    void Initialize();

    bool StartRecording();
    void StopRecording();
    bool IsRecording() const;

    static RecordingMode GetMode();
    // OBSController* GetOBSController() { return &m_obsController; }
    NativeRecorder* GetNativeRecorder() const { return m_nativeRecorder.get(); }

    // void ShowSettingsDialog();
    // void ShowCloseOBSDialog();

private:
    // OBSController m_obsController;
    std::unique_ptr<NativeRecorder> m_nativeRecorder;

    bool m_obsWasStartedByUs = false;
};
