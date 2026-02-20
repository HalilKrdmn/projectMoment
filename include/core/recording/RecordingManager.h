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

    // ── Recording ───────────────────────────────────────────────────────────
    void StartRecording() const;
    void StopRecording() const;
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
    int m_clipDuration;
};