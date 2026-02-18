#include "core/recording/RecordingManager.h"

#include "core/CoreServices.h"

RecordingManager::RecordingManager() {
    m_nativeRecorder = std::make_unique<NativeRecorder>();
}

RecordingManager::~RecordingManager() {
    StopRecording();
}

void RecordingManager::Initialize() {
    printf("[RecordingManager] Initialize: Step 1 - Getting services\n");
    auto& services = CoreServices::Instance();

    printf("[RecordingManager] Initialize: Step 2 - Getting config\n");
    const Config* config = services.GetConfig();

    if (!config) {
        printf("[RecordingManager] Config not available\n");
        return;
    }

    printf("[RecordingManager] Initialize: Step 3 - Recording mode: %s\n",
           config->recordingMode.c_str());

    // Native recorder setup - sadece değer varsa set et
    if (!config->nativeAudioInputDevice.empty() &&
        config->nativeAudioInputDevice != "default") {
        printf("[RecordingManager] Initialize: Step 4 - Setting audio input: %s\n",
               config->nativeAudioInputDevice.c_str());
        m_nativeRecorder->SetAudioInputDevice(config->nativeAudioInputDevice);
    } else {
        printf("[RecordingManager] Initialize: Step 4 - Skipping audio input (empty or default)\n");
    }

    if (!config->nativeAudioOutputDevice.empty() &&
        config->nativeAudioOutputDevice != "default") {
        printf("[RecordingManager] Initialize: Step 5 - Setting audio output: %s\n",
               config->nativeAudioOutputDevice.c_str());
        m_nativeRecorder->SetAudioOutputDevice(config->nativeAudioOutputDevice);
    } else {
        printf("[RecordingManager] Initialize: Step 5 - Skipping audio output (empty or default)\n");
    }

    if (!config->nativeScreenOutput.empty()) {
        printf("[RecordingManager] Initialize: Step 6 - Setting screen: %s\n",
               config->nativeScreenOutput.c_str());
        m_nativeRecorder->SetScreen(config->nativeScreenOutput);
    } else {
        printf("[RecordingManager] Initialize: Step 6 - Skipping screen (empty)\n");
    }

    if (!config->nativeVideoCodec.empty()) {
        printf("[RecordingManager] Initialize: Step 7 - Setting video codec: %s\n",
               config->nativeVideoCodec.c_str());
        m_nativeRecorder->SetVideoCodec(config->nativeVideoCodec);
    }

    if (!config->nativeAudioCodec.empty()) {
        printf("[RecordingManager] Initialize: Step 8 - Setting audio codec: %s\n",
               config->nativeAudioCodec.c_str());
        m_nativeRecorder->SetAudioCodec(config->nativeAudioCodec);
    }

    if (config->nativeVideoBitrate > 0 || config->nativeAudioBitrate > 0) {
        printf("[RecordingManager] Initialize: Step 9 - Setting bitrates: %d/%d\n",
               config->nativeVideoBitrate, config->nativeAudioBitrate);
        m_nativeRecorder->SetBitrates(config->nativeVideoBitrate, config->nativeAudioBitrate);
    }

    if (config->nativeFPS > 0) {
        printf("[RecordingManager] Initialize: Step 10 - Setting FPS: %d\n", config->nativeFPS);
        m_nativeRecorder->SetFPS(config->nativeFPS);
    }

    printf("[RecordingManager] Initialize: Step 11 - Setting callback\n");
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

