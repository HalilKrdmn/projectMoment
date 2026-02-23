#include "core/recording/RecordingManager.h"

#include "core/CoreServices.h"

#include <filesystem>

RecordingManager::RecordingManager() : m_clipDuration(60) {
    m_nativeRecorder = std::make_unique<NativeRecorder>();
}

RecordingManager::~RecordingManager() {
    if (m_nativeRecorder) m_nativeRecorder->StopRecording();
}

// ─── Initialize ───────────────────────────────────────────────────────────────
void RecordingManager::Initialize() {
    const Config* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    printf("[RecordingManager] Initialize — mode: %s\n", cfg->recordingMode.c_str());

    m_clipDuration = (cfg->nativeClipDuration > 0) ? cfg->nativeClipDuration : 60;
    ApplyConfig();

    if (cfg->recordingAutoStart && GetMode() == RecordingMode::NATIVE)
        StartRecording();

    printf("[RecordingManager] Initialize: OKAY (clipDuration=%ds)\n", m_clipDuration);
}

// ─── Recording ────────────────────────────────────────────────────────────────
void RecordingManager::StartRecording() {
    if (GetMode() == RecordingMode::OBS) return;
    if (!m_nativeRecorder) return;

    m_nativeRecorder->SetClipDuration(m_clipDuration);
    m_nativeRecorder->SetOnClipSaved([this](const fs::path& p, const bool ok) {
        printf("[RecordingManager] Clip %s: %s\n", ok ? "saved" : "FAIL", p.c_str());
        if (ok && m_onClipSaved) m_onClipSaved(p);
    });

    if (m_nativeRecorder->StartRecording())
        printf("[RecordingManager] Recording started (%ds buffer)\n", m_clipDuration);
    else
        printf("[RecordingManager] Unable to start recording!\n");
}

void RecordingManager::StopRecording() {
    if (GetMode() == RecordingMode::OBS) return;
    if (m_nativeRecorder) m_nativeRecorder->StopRecording();
    printf("[RecordingManager] Recording stopped\n");
}

bool RecordingManager::IsRecording() const {
    if (GetMode() == RecordingMode::OBS) return false;
    return m_nativeRecorder && m_nativeRecorder->IsRecording();
}

// ─── Clip ─────────────────────────────────────────────────────────────────────
void RecordingManager::SaveClip() const {
    if (GetMode() == RecordingMode::OBS) return;
    if (m_nativeRecorder) m_nativeRecorder->SaveClip();
}

bool RecordingManager::IsSavingClip() const {
    return m_nativeRecorder && m_nativeRecorder->IsSaving();
}

void RecordingManager::SetOnClipSaved(std::function<void(const fs::path&)> cb) {
    m_onClipSaved = std::move(cb);
}

// ─── ApplyConfig ──────────────────────────────────────────────────────────────
void RecordingManager::ApplyConfig() const {
    const Config* cfg = CoreServices::Instance().GetConfig();
    if (!cfg || !m_nativeRecorder) return;

    auto* r = m_nativeRecorder.get();

    // Audio tracks
    if (!cfg->nativeAudioTracks.empty())
        r->SetAudioTracks(cfg->nativeAudioTracks);

    // Screen
    if (!cfg->nativeScreenOutput.empty())
        r->SetScreen(cfg->nativeScreenOutput);

    // Video
    r->SetVideoCodec(cfg->nativeVideoCodec);
    r->SetFPS(cfg->nativeFPS);
    r->SetQuality(cfg->nativeQuality);
    r->SetBitrateMode(cfg->nativeBitrateMode);
    r->SetVideoBitrate(cfg->nativeVideoBitrate);
    r->SetFramerateMode(cfg->nativeFramerateMode);
    r->SetColorRange(cfg->nativeColorRange);
    r->SetTune(cfg->nativeTune);
    r->SetContainerFormat(cfg->nativeContainerFormat);

    // Audio
    r->SetAudioCodec(cfg->nativeAudioCodec);
    r->SetAudioBitrate(cfg->nativeAudioBitrate);

    // Encoder
    r->SetEncoder(cfg->nativeEncoder);
    r->SetFallbackCpu(cfg->nativeFallbackCpu);

    // Replay buffer
    r->SetClipDuration(m_clipDuration);
    r->SetReplayStorage(cfg->nativeReplayStorage);

    // Misc
    r->SetShowCursor(cfg->nativeShowCursor);

    // Output
    r->SetOutputDirectory(cfg->libraryPath);
    r->SetStatusCallback([](const std::string& s) {
        printf("[RecordingManager] %s\n", s.c_str());
    });

    printf("[NativeRecorder] Audio tracks configured: %zu\n", cfg->nativeAudioTracks.size());
    for (size_t i = 0; i < cfg->nativeAudioTracks.size(); i++) {
        const auto&[name, device, deviceType] = cfg->nativeAudioTracks[i];
        printf("[NativeRecorder]   [%zu] %s | device=%s | type=%s\n",
               i+1,
               name.empty()   ? "(unnamed)" : name.c_str(),
               device.empty() ? "(default)" : device.c_str(),
               (deviceType == AudioDeviceType::Input) ? "Input" : "Output");
    }
}

RecordingMode RecordingManager::GetMode() {
    if (const Config* cfg = CoreServices::Instance().GetConfig(); cfg && cfg->recordingMode == "obs") return RecordingMode::OBS;
    return RecordingMode::NATIVE;
}