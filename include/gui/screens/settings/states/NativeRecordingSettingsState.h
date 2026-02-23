#pragma once

#include "core/recording/NativeRecorder.h"
#include "core/media/AudioDeviceEnumerator.h"

#include <vector>
#include <string>

// ──────────────────────────────────────────────────────────────────────────────
struct SelectedTrack {
    std::string     customName;
    std::string     deviceId;
    AudioDeviceType deviceType;

    bool operator==(const SelectedTrack& o) const {
        return customName == o.customName && deviceId == o.deviceId && deviceType == o.deviceType;
    }
};
// ──────────────────────────────────────────────────────────────────────────────

class NativeRecordingSettingsState {
public:
    NativeRecordingSettingsState();
    void Draw();

    bool IsDirty() const { return m_dirty; }
    void Save()          { SaveChanges(); SyncOriginals(); m_dirty = false; }

private:
    // ── Device lists ──────────────────────────────────────────────────────────
    std::vector<AudioDevice> m_inputDevices;
    std::vector<AudioDevice> m_outputDevices;
    std::vector<ScreenInfo>  m_screens;
    int                      m_selectedScreenIdx = 0;
    char                     m_screenOutput[128] = {};

    // ───────────────────────────────────
    VideoCodec      m_videoCodec       = VideoCodec::H264;
    AudioCodec      m_audioCodec       = AudioCodec::OPUS;
    EncoderMode     m_encoderMode      = EncoderMode::GPU;
    bool            m_fallbackCpu      = true;
    QualityPreset   m_quality          = QualityPreset::VeryHigh;
    BitrateMode     m_bitrateMode      = BitrateMode::Auto;
    int             m_videoBitrate     = 5000;
    int             m_audioBitrate     = 192;
    int             m_fps              = 60;
    int             m_replayDuration   = 60;
    ReplayStorage   m_replayStorage    = ReplayStorage::RAM;
    bool            m_showCursor       = true;
    ContainerFormat m_containerFmt     = ContainerFormat::MP4;
    ColorRange      m_colorRange       = ColorRange::Limited;
    FramerateMode   m_framerateMode    = FramerateMode::VFR;
    TuneProfile     m_tune             = TuneProfile::Quality;
    AudioMode       m_audioMode        = AudioMode::Mixed;
    std::vector<SelectedTrack> m_inputTracks;
    std::vector<SelectedTrack> m_outputTracks;
    // ───────────────────────────────────
    VideoCodec      m_origVideoCodec;
    AudioCodec      m_origAudioCodec;
    EncoderMode     m_origEncoderMode;
    bool            m_origFallbackCpu      = true;
    QualityPreset   m_origQuality;
    BitrateMode     m_origBitrateMode;
    int             m_origVideoBitrate     = 5000;
    int             m_origAudioBitrate     = 192;
    int             m_origFps              = 60;
    int             m_origReplayDuration   = 60;
    ReplayStorage   m_origReplayStorage    = ReplayStorage::RAM;
    bool            m_origShowCursor       = true;
    ContainerFormat m_origContainerFmt;
    ColorRange      m_origColorRange;
    FramerateMode   m_origFramerateMode;
    TuneProfile     m_origTune;
    AudioMode       m_origAudioMode;
    int             m_origSelectedScreenIdx = 0;
    char            m_origScreenOutput[128] = {};
    std::vector<SelectedTrack> m_origInputTracks;
    std::vector<SelectedTrack> m_origOutputTracks;
    // ───────────────────────────────────

    bool m_dirty = false;

    // ── Draw sections ─────────────────────────────────────────────────────────
    void DrawEncoderSection();
    void DrawAudioSection();
    void DrawVideoSection();
    void DrawScreenSelector(float labelW);
    void DrawRecordingSettings(float labelW);
    void DrawReplayBufferSection(float labelW);
    void DrawDeviceColumn(
        const char* id, const char* label,
        const std::vector<AudioDevice>& devs,
        std::vector<SelectedTrack>& tracks);

    // ── Helpers ───────────────────────────────────────────────────────────────
    void RefreshDeviceLists();
    void LoadFromConfig();
    void SaveChanges() const;
    void SyncOriginals();
    void CheckDirty();

    static std::vector<SelectedTrack> TrackListFromConfig(
        const std::vector<AudioTrack>& src, AudioDeviceType type);
    static std::vector<AudioTrack> TrackListToConfig(
        const std::vector<SelectedTrack>& in,
        const std::vector<SelectedTrack>& out);
};