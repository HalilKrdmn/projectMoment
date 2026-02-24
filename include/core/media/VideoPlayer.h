#pragma once

#include "core/VideoInfo.h"

#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <imgui.h>

class VideoPlayer {
public:
    VideoPlayer() = default;
    ~VideoPlayer();

    // Load video file
    bool LoadVideo(const std::string& filePath);

    // Player controls
    void Play();
    void Pause();
    void Stop();
    void Seek(double seconds);

    // Playback info
    bool IsPlaying() const { return m_isPlaying; }
    double GetCurrentTime() const { return m_currentTime; }
    double GetDuration() const { return m_duration; }
    double GetProgress() const { return m_duration > 0 ? m_currentTime / m_duration : 0.0; }

    // Render frame to ImGui texture
    void Update(float deltaTime);
    ImTextureID GetFrameTexture() const { return static_cast<ImTextureID>(static_cast<intptr_t>(m_textureId)); }

    // Video info
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    // FFmpeg context
    AVFormatContext* m_formatCtx = nullptr;
    AVCodecContext* m_codecCtx = nullptr;
    AVStream* m_videoStream = nullptr;
    int m_videoStreamIndex = -1;

    // Frame processing
    AVFrame* m_frame = nullptr;
    AVFrame* m_rgbFrame = nullptr;
    SwsContext* m_swsCtx = nullptr;
    uint8_t* m_buffer = nullptr;

    // OpenGL texture
    unsigned int m_textureId = 0;

    // Playback state
    bool m_isPlaying = false;
    bool m_isLoaded = false;
    double m_currentTime = 0.0;
    double m_duration = 0.0;
    double m_frameRate = 30.0;
    double m_frameTime = 1.0 / 30.0;
    double m_frameAccumulator = 0.0;

    // Video properties
    int m_width = 0;
    int m_height = 0;

    // Helper methods
    void Cleanup();
    void CreateTexture();
    void UpdateFrame();
    void DecodeFrame();
};