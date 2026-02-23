#pragma once

#include "core/media/AudioAnalyzer.h"
#include "core/media/VideoPlayer.h"
#include "data/VideoInfo.h"

#include <memory>
#include <mutex>
#include <chrono>
#include <string>

struct ImVec2;
class EditingScreen;

class VideoEditState {
public:
    ~VideoEditState();
    void Draw(const EditingScreen* parent);

    float GetSelectStart()   const { return m_selectStart; }
    float GetSelectEnd()     const { return m_selectEnd; }
    float GetTotalDuration() const {
        return m_videoPlayer ? (float)m_videoPlayer->GetDuration() : 0.0f;
    }

private:
    void DrawInfoBar(const EditingScreen* parent, const VideoInfo& video) const;
    void DrawVideoPlayer() const;
    void DrawTimeline(const EditingScreen* parent, const VideoInfo& video);
    void DrawTimelineHeader(const EditingScreen* parent, ImVec2 pos, ImVec2 size);
    void DrawEmptyTrackBoxFull(ImVec2 pos, ImVec2 size, const char* label);
    void DrawTrackBoxFull(ImVec2 pos, ImVec2 size, const char* label, int trackIndex) const;

    std::unique_ptr<VideoPlayer>   m_videoPlayer;

    mutable std::mutex             m_analyzerMutex;
    std::unique_ptr<AudioAnalyzer> m_audioAnalyzer;
    std::unique_ptr<AudioAnalyzer> m_pendingAnalyzer;

    std::string m_lastLoadedPath;
    bool        m_isPlaying        = false;
    float       m_playbackProgress = 0.0f;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastFrameTime;

    float m_selectStart = 0.0f;
    float m_selectEnd   = 1.0f;
};