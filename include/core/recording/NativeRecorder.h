#pragma once

#include "core/Config.h"

#include <atomic>
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

namespace fs = std::filesystem;
 // ──────────────────────────────────────────────────────────────────────────

class NativeRecorder {
public:
    using OnClipSaved = std::function<void(const fs::path& outputPath, bool success)>;

    NativeRecorder();
    ~NativeRecorder();

    // ── Screen / hardware detection ─────────────────────────────────────────
    static std::vector<ScreenInfo> GetScreens();
    static bool                    CheckDependencies();
    static std::string             GetCompositorType();
    static bool                    IsAvailable();
    static const std::vector<int>& GetDurationOptions();

    // ── Configuration ───────────────────────────────────────────────────────
    void SetAudioTracks(const std::vector<AudioTrack>& tracks);
    void SetScreen(const std::string& output);
    void SetVideoCodec(VideoCodec codec);
    void SetAudioCodec(AudioCodec codec);
    void SetEncoder(EncoderMode encoder);
    void SetFallbackCpu(bool fallback);
    void SetQuality(QualityPreset quality);
    void SetBitrateMode(BitrateMode mode);
    void SetVideoBitrate(int kbps);
    void SetAudioBitrate(int kbps);
    void SetFPS(int fps);
    void SetClipDuration(int seconds);
    void SetReplayStorage(ReplayStorage storage);
    void SetShowCursor(bool show);
    void SetContainerFormat(ContainerFormat fmt);
    void SetColorRange(ColorRange cr);
    void SetFramerateMode(FramerateMode fm);
    void SetTune(TuneProfile tune);
    void SetOutputDirectory(const fs::path& dir);
    void SetOnClipSaved(OnClipSaved cb) { m_onClipSaved = std::move(cb); }
    void SetStatusCallback(const std::function<void(const std::string&)>& cb) { m_statusCallback = cb; }

    // ── Recording control ──────────────────────────────────────────────────
    bool StartRecording();
    void StopRecording();
    bool IsRecording() const { return m_recording; }

    // ── Save clip ──────────────────────────────────────────────────────────
    void SaveClip();
    bool IsSaving() const { return m_saving; }

    // ── Info ───────────────────────────────────────────────────────────────
    float GetBufferedSeconds() const { return m_recording ? (float)m_clipDuration : 0.0f; }
    std::string GetStatus()    const { return m_status; }

private:
    std::string BuildCommand() const;
    static bool ExecuteCommand(const std::string& cmd, std::string& out);
    void        UpdateStatus(const std::string& status);
    static std::string MakeTimestampName();

    // ── Config values ───────────────────────────────────────────────────────
    std::vector<AudioTrack> m_audioTracks;
    std::string     m_screenOutput;
    VideoCodec      m_videoCodec      = VideoCodec::H264;
    AudioCodec      m_audioCodec      = AudioCodec::OPUS;
    EncoderMode     m_encoder         = EncoderMode::GPU;
    bool            m_fallbackCpu     = true;
    QualityPreset   m_quality         = QualityPreset::VeryHigh;
    BitrateMode     m_bitrateMode     = BitrateMode::Auto;
    int             m_videoBitrate    = 5000;
    int             m_audioBitrate    = 192;
    int             m_fps             = 60;
    int             m_clipDuration    = 60;
    ReplayStorage   m_replayStorage   = ReplayStorage::RAM;
    bool            m_showCursor      = true;
    ContainerFormat m_containerFormat = ContainerFormat::MP4;
    ColorRange      m_colorRange      = ColorRange::Limited;
    FramerateMode   m_framerateMode   = FramerateMode::VFR;
    TuneProfile     m_tune            = TuneProfile::Quality;
    fs::path        m_outputDir;

    // ── State ────────────────────────────────────────────────────────────────
    std::atomic<bool> m_recording{false};
    std::atomic<bool> m_saving   {false};
    pid_t             m_gsrPid   = -1;

    std::string                              m_status = "Ready";
    std::function<void(const std::string&)>  m_statusCallback;
    OnClipSaved                              m_onClipSaved;
};