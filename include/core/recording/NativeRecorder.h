#pragma once

#include "core/Config.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────
struct ScreenInfo {
    std::string name;
    std::string output;
    int width       = 0;
    int height      = 0;
    int x           = 0;
    int y           = 0;
    int refreshRate = 60;
};

struct ReplaySegment {
    fs::path                              path;
    std::chrono::steady_clock::time_point createdAt;
    float                                 durationSec = 0.0f;
};

namespace fs = std::filesystem;
// ──────────────────────────────────────────────────────────────────────────

class NativeRecorder {
public:
    using OnClipSaved = std::function<void(const fs::path& outputPath, bool success)>;

    NativeRecorder();
    ~NativeRecorder();


    // ── Screen / hardware detection ────────────────────────────────────────
    static std::vector<ScreenInfo>  GetScreens();
    static bool                     CheckDependencies();
    static std::string              GetCompositorType();

    // ── Configuration ───────────────────────────────────────────────────────
    void SetAudioTracks(const std::vector<AudioTrack>& tracks);
    void SetScreen(const std::string& output);
    void SetVideoCodec(const std::string& codec);
    void SetAudioCodec(const std::string& codec);
    void SetBitrates(int videoBitrate, int audioBitrate);
    void SetFPS(int fps);
    void SetClipDuration(int seconds);
    void SetOutputDirectory(const fs::path& dir);
    void SetOnClipSaved(OnClipSaved cb){ m_onClipSaved = std::move(cb); }

    // ── Recording control ────────────────────────────────────────────────────
    bool StartRecording();
    void StopRecording();
    bool IsRecording() const { return m_recording; }

    // ── Save clip ─────────────────────────────────────────────────────────────
    void SaveClip();
    bool IsSaving() const { return m_saving; }

    // ── Info ─────────────────────────────────────────────────────────────────
    float GetBufferedSeconds() const;
    int   GetSegmentCount()    const;
    static const std::vector<int>& GetDurationOptions(); // {30,60,120,180,240,300}

    // ── Status ────────────────────────────────────────────────────────────────
    std::string GetStatus() const { return m_status; }
    void SetStatusCallback(const std::function<void(const std::string&)>& cb) { m_statusCallback = cb; }

private:
    void SegmentLoop();
    bool StartNextSegment();
    void StopCurrentSegment();
    void PruneOldSegments();
    bool MergeSegments(const fs::path& out);

    static bool        ExecuteCommand(const std::string& cmd, std::string& out);
    void               UpdateStatus(const std::string& status);
    static fs::path    MakeTempDir();
    static std::string MakeTimestampName();
    static std::string BuildWfRecorderCmd(const std::string& screenOutput,
                                          const std::vector<AudioTrack>& audioTracks,
                                          const std::string& videoCodec,
                                          int fps,
                                          const std::string& outputPath);

    // ── Config ────────────────────────────────────────────────────────────────
    std::vector<AudioTrack> m_audioTracks;
    std::string             m_screenOutput;
    std::string             m_videoCodec;
    std::string             m_audioCodec;
    int                     m_videoBitrate;
    int                     m_audioBitrate;
    int                     m_fps;
    int                     m_clipDuration;
    fs::path                m_outputDir;
    fs::path                m_tempDir;


    int m_marginSec = 1;
    int m_segDuration = 1;

    // ── State ─────────────────────────────────────────────────────────────────
    std::atomic<bool> m_recording{false};
    std::atomic<bool> m_saving  {false};
    std::thread       m_thread;

    pid_t    m_segPid = -1;
    fs::path m_currentSegPath;

    mutable std::mutex        m_segMutex;
    std::deque<ReplaySegment> m_segments;

    std::string m_status = "Ready";
    std::function<void(const std::string&)> m_statusCallback;
    OnClipSaved                             m_onClipSaved;
};