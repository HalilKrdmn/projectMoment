#pragma once

#include "data/VideoInfo.h"
#include "core/media/VideoPlayer.h"
#include "core/media/AudioAnalyzer.h"

#include <memory>
#include <string>
#include <chrono>

class EditingScreen;

class VideoEditState {
public:
    VideoEditState() = default;
    ~VideoEditState();

    void Draw(EditingScreen *parent);

private:
    void DrawVideoPlayer() const;
    void DrawTimeline(const VideoInfo& video);
    void DrawTimelineHeader(ImVec2 pos, ImVec2 size);
    void DrawEmptyTrackBoxFull(ImVec2 pos, ImVec2 size, const char* label) const;
    void DrawTrackBoxFull(ImVec2 pos, ImVec2 size, const char* label, int trackIndex) const;
    void DrawInfoBar(const VideoInfo& video);

    std::unique_ptr<VideoPlayer> m_videoPlayer;
    std::unique_ptr<AudioAnalyzer> m_audioAnalyzer;

    std::string m_lastLoadedPath;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    
    float m_playbackProgress = 0.0f;
    bool m_isPlaying = false;
};