#pragma once

#include <string>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

// NOTE: Named WaveformTrack (not AudioTrack) to avoid ODR collision with
// the AudioTrack struct defined in the recording/config subsystem.
struct WaveformTrack {
    int                streamIndex  = -1;
    std::string        name;
    std::vector<float> waveform;       // normalized [0,1] per second
    float              maxPeakLevel   = 0.001f;
};

class AudioAnalyzer {
public:
    ~AudioAnalyzer();

    bool LoadAndComputeTimeline(const std::string& filePath, double videoDuration);

    int  GetTrackCount()   const { return (int)m_tracks.size(); }
    int  GetTotalSeconds() const;

    const std::vector<WaveformTrack>& GetTracks()              const { return m_tracks; }
    const std::vector<float>&         GetWaveform(int idx)     const;

    bool  IsTrackSilent(int trackIndex, float threshold = 0.01f) const;
    float GetGlobalMaxPeak() const { return m_globalMaxPeak; }

private:
    // Internal per-track processing state — never in the public header
    struct TrackCtx {
        int             streamIndex = -1;
        AVCodecContext* codecCtx    = nullptr;
        SwrContext*     swrCtx      = nullptr;
        std::vector<float> rawRms;
        float           maxPeak     = 0.001f;
    };

    void ExtractAudioTracks();
    void PreComputeTimeline();
    void Cleanup();

    AVFormatContext*          m_formatCtx     = nullptr;
    double                    m_videoDuration = 0.0;
    float                     m_globalMaxPeak = 0.001f;

    std::vector<WaveformTrack> m_tracks;  // final output
    std::vector<TrackCtx>      m_ctx;     // private working state
};