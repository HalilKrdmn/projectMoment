#include "core/recording/RecordingManager.h"

#include "core/CoreServices.h"

RecordingManager::RecordingManager() {
    m_nativeRecorder = std::make_unique<NativeRecorder>();
}

RecordingManager::~RecordingManager() {
    StopRecording();
}

void RecordingManager::Initialize() {
    auto& services = CoreServices::Instance();
    const Config* config = services.GetConfig();

    if (!config) {
        printf("[RecordingManager] Config not available\n");
        return;
    }

    printf("[RecordingManager] Initialize - Recording mode: %s\n", config->recordingMode.c_str());

    // Audio tracks
    if (!config->nativeAudioTracks.empty()) {
        m_nativeRecorder->SetAudioTracks(config->nativeAudioTracks);
        printf("[RecordingManager] Initialize - Audio tracks: %zu\n", config->nativeAudioTracks.size());
    }

    if (!config->nativeScreenOutput.empty()) {
        m_nativeRecorder->SetScreen(config->nativeScreenOutput);
    }

    if (!config->nativeVideoCodec.empty()) {
        m_nativeRecorder->SetVideoCodec(config->nativeVideoCodec);
    }

    if (!config->nativeAudioCodec.empty()) {
        m_nativeRecorder->SetAudioCodec(config->nativeAudioCodec);
    }

    if (config->nativeVideoBitrate > 0 || config->nativeAudioBitrate > 0) {
        m_nativeRecorder->SetBitrates(config->nativeVideoBitrate, config->nativeAudioBitrate);
    }

    if (config->nativeFPS > 0) {
        m_nativeRecorder->SetFPS(config->nativeFPS);
    }

    m_nativeRecorder->SetStatusCallback([](const std::string& status) {
        printf("[RecordingManager] Status: %s\n", status.c_str());
    });

    printf("[RecordingManager] Initialize: COMPLETE\n");
}


bool RecordingManager::StartRecording() {
    auto& services = CoreServices::Instance();
    const Config* config = services.GetConfig();

    if (!config) return false;

    if (config->recordingMode == "obs") {
        printf("[RecordingManager] OBS mode not implemented yet\n");
        return false;
    }
    // Native recording
    const std::string outputPath = config->libraryPath + "/recording_" +
                             std::to_string(time(nullptr)) + ".mp4";

    return m_nativeRecorder->StartRecording(outputPath);
}

void RecordingManager::StopRecording() {
    auto& services = CoreServices::Instance();
    const Config* config = services.GetConfig();

    if (!config) return;

    if (config->recordingMode == "obs") {
        // m_obsController.StopRecording();
    } else {
        m_nativeRecorder->StopRecording();
    }
}

bool RecordingManager::IsRecording() const {
    // Config* config = GetConfig();
    // if (config && config->recordingMode == "obs") {
    //     return m_obsController.IsRecording();
    // }

    return m_nativeRecorder->IsRecording();
}

RecordingMode RecordingManager::GetMode() {
    if (const Config* config = CoreServices::Instance().GetConfig(); config && config->recordingMode == "obs") {
        return RecordingMode::OBS;
    }
    return RecordingMode::NATIVE;
}

