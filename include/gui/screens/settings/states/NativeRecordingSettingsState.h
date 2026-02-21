#pragma once

#include "core/recording/NativeRecorder.h"

#include <vector>
#include <string>

// ──────────────────────────────────────────────────────────────────────────
struct SelectedTrack {
    std::string     customName;
    std::string     deviceId;
    AudioDeviceType deviceType;

    bool operator==(const SelectedTrack& other) const {
        return customName == other.customName
            && deviceId   == other.deviceId
            && deviceType == other.deviceType;
    }
};
// ──────────────────────────────────────────────────────────────────────────

class NativeRecordingSettingsState {
public:
    NativeRecordingSettingsState();
    void Draw();

    bool IsDirty() const { return m_dirty; }
    void Save()          { SaveChanges(); SyncOriginals(); m_dirty = false; }

private:
    // ── Current values ───────────────────────────────────────────────────────
    std::vector<AudioDevice>   m_inputDevices;
    std::vector<AudioDevice>   m_outputDevices;
    std::vector<ScreenInfo>    m_screens;
    int                        m_selectedScreenIdx = 0;

    // ───────────────────────────────────
    int  m_audioModeIndex    = 0;
    char m_screenOutput[128] = {};
    char m_videoCodec[64]    = {};
    char m_audioCodec[64]    = {};
    int  m_videoBitrate      = 8000;
    int  m_audioBitrate      = 192;
    int  m_fps               = 60;
    int  m_replayDuration    = 30;
    std::vector<SelectedTrack> m_inputTracks;
    std::vector<SelectedTrack> m_outputTracks;
    // ───────────────────────────────────
    int  m_origAudioModeIndex    = 0;
    int  m_origSelectedScreenIdx = 0;
    char m_origScreenOutput[128] = {};
    char m_origVideoCodec[64]    = {};
    char m_origAudioCodec[64]    = {};
    int  m_origVideoBitrate      = 8000;
    int  m_origAudioBitrate      = 192;
    int  m_origFps               = 60;
    int  m_origReplayDuration    = 30;
    std::vector<SelectedTrack> m_origInputTracks;
    std::vector<SelectedTrack> m_origOutputTracks;
    // ───────────────────────────────────

    bool m_dirty = false;

    // ── Regulation state ───────────────────────────────────────────────────────
    int  m_editingInputIdx  = -1;
    int  m_editingOutputIdx = -1;

    // ── Methods ──────────────────────────────────────────────────────────────
    void RefreshDeviceLists();
    void LoadFromConfig();
    void SaveChanges() const;
    void SyncOriginals();
    void CheckDirty();

    void DrawAudioSection();
    void DrawVideoSection();
    void DrawDeviceColumn(
        const char *id,
        const char *label,
        const std::vector<AudioDevice> &devs,
        std::vector<SelectedTrack> &tracks
    );

    static std::vector<SelectedTrack> TrackListFromConfig(
        const std::vector<AudioTrack>& src, AudioDeviceType type);
    static std::vector<AudioTrack> TrackListToConfig(
        const std::vector<SelectedTrack>& in,
        const std::vector<SelectedTrack>& out);
};