#pragma once

#include "core/recording/NativeRecorder.h"

#include <memory>

// ──────────────────────────────────────────────────────────────────────────
enum class RecordingMode { NATIVE, OBS };
// ──────────────────────────────────────────────────────────────────────────

class RecordingManager {
public:
    RecordingManager();
    ~RecordingManager();

    void Initialize();
    void SetOnClipSaved(std::function<void(const fs::path&)> cb);

    // ── Recording ───────────────────────────────────────────────────────────
    void StartRecording();
    void StopRecording();
    bool IsRecording() const;

    // ── Clip ─────────────────────────────────────────────────────────────────
    void SaveClip() const;
    bool IsSavingClip() const;

    // ── Assistants ───────────────────────────────────────────────────────────
    static RecordingMode GetMode();
    // GetOBSRecorder
    NativeRecorder* GetNativeRecorder() const { return m_nativeRecorder.get(); }

private:
    void ApplyConfig() const;

    std::unique_ptr<NativeRecorder> m_nativeRecorder;
    std::function<void(const fs::path&)> m_onClipSaved;
    int m_clipDuration;
};