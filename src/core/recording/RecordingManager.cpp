#include "core/recording/RecordingManager.h"


RecordingManager::RecordingManager() {
}

RecordingManager::~RecordingManager() {
    StopRecording();
}

void RecordingManager::Initialize() {
    const Config* config = GetConfig();

    if (!config) {
        printf("[RecordingManager] Config not available\n");
        return;
    }

    printf("[RecordingManager] Initializing with mode: %s\n", config->recordingMode.c_str());

    // Native recorder setup
    m_nativeRecorder.SetAudioInputDevice(config->nativeAudioInputDevice);
    m_nativeRecorder.SetAudioOutputDevice(config->nativeAudioOutputDevice);
    m_nativeRecorder.SetScreen(config->nativeScreenOutput);
    m_nativeRecorder.SetVideoCodec(config->nativeVideoCodec);
    m_nativeRecorder.SetAudioCodec(config->nativeAudioCodec);
    m_nativeRecorder.SetBitrates(config->nativeVideoBitrate, config->nativeAudioBitrate);
    m_nativeRecorder.SetFPS(config->nativeFPS);

    // Status callback
    m_nativeRecorder.SetStatusCallback([](const std::string& status) {
        printf("[RecordingManager] Status: %s\n", status.c_str());
    });
}

bool RecordingManager::StartRecording() {
    const Config* config = GetConfig();
    if (!config) return false;

    if (config->recordingMode == "obs") {
        printf("[RecordingManager] OBS mode not implemented yet\n");
        return false;
    }
    // Native recording
    const std::string outputPath = config->libraryPath + "/recording_" +
                             std::to_string(time(nullptr)) + ".mp4";

    return m_nativeRecorder.StartRecording(outputPath);
}

void RecordingManager::StopRecording() {
    const Config* config = GetConfig();
    if (!config) return;

    if (config->recordingMode == "obs") {
        // m_obsController.StopRecording();
    } else {
        m_nativeRecorder.StopRecording();
    }
}

bool RecordingManager::IsRecording() const {
    // Config* config = GetConfig();
    // if (config && config->recordingMode == "obs") {
    //     return m_obsController.IsRecording();
    // }

    return m_nativeRecorder.IsRecording();
}

RecordingMode RecordingManager::GetMode() {
    Config* config = CoreServices::Instance().GetConfig();
    if (config && config->recordingMode == "obs") {
        return RecordingMode::OBS;
    }
    return RecordingMode::NATIVE;
}

