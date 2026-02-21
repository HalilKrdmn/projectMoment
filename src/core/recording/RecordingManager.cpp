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

    printf("[RecordingManager] Initialize — mod: %s\n", cfg->recordingMode.c_str());

    // clipDuration 0 ise makul bir varsayılan kullan
    m_clipDuration = (cfg->clipDuration > 0) ? cfg->clipDuration : 60;
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
        printf("[RecordingManager] Clip %s: %s\n", ok ? "recorded" : "FAIL", p.c_str());
        if (ok && m_onClipSaved) m_onClipSaved(p);
    });

    if (m_nativeRecorder->StartRecording())
        printf("[RecordingManager] Recording has begun (%ds clip)\n", m_clipDuration);
    else
        printf("[RecordingManager] Unable to start recording!\n");
}

void RecordingManager::StopRecording() {
    if (GetMode() == RecordingMode::OBS) {
        // m_obsController.StopReplayBuffer();
        printf("[RecordingManager] Recording stopped\n");
        return;
    }
    if (m_nativeRecorder) {
        m_nativeRecorder->StopRecording();
        printf("[RecordingManager] Recording stopped\n");
    }
}

bool RecordingManager::IsRecording() const {
    if (GetMode() == RecordingMode::OBS) return false;
    return m_nativeRecorder && m_nativeRecorder->IsRecording();
}

// ─── Clip ────────────────────────────────────────────────────────────────────
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

// ─── Helpers ──────────────────────────────────────────────────────────────────
void RecordingManager::ApplyConfig() const {
    const Config* cfg = CoreServices::Instance().GetConfig();
    if (!cfg || !m_nativeRecorder) return;

    if (!cfg->nativeAudioTracks.empty())
        m_nativeRecorder->SetAudioTracks(cfg->nativeAudioTracks);
    if (!cfg->nativeScreenOutput.empty())
        m_nativeRecorder->SetScreen(cfg->nativeScreenOutput);
    if (!cfg->nativeVideoCodec.empty())
        m_nativeRecorder->SetVideoCodec(cfg->nativeVideoCodec);
    if (!cfg->nativeAudioCodec.empty())
        m_nativeRecorder->SetAudioCodec(cfg->nativeAudioCodec);
    if (cfg->nativeVideoBitrate > 0 || cfg->nativeAudioBitrate > 0)
        m_nativeRecorder->SetBitrates(cfg->nativeVideoBitrate, cfg->nativeAudioBitrate);
    if (cfg->nativeFPS > 0)
        m_nativeRecorder->SetFPS(cfg->nativeFPS);

    m_nativeRecorder->SetOutputDirectory(cfg->libraryPath);
    m_nativeRecorder->SetStatusCallback([](const std::string& s) {
        printf("[RecordingManager] %s\n", s.c_str());
    });
}

RecordingMode RecordingManager::GetMode() {
    if (const Config* cfg = CoreServices::Instance().GetConfig(); cfg && cfg->recordingMode == "obs") return RecordingMode::OBS;
    return RecordingMode::NATIVE;
}