#pragma once

#include <vector>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

struct AudioTrack {
    std::string name;

    std::vector<float> waveform;  // ✅ ONE value per second

    float maxPeakLevel;
    int streamIndex;
    AVCodecContext* codecCtx;
};

class AudioAnalyzer {
public:
    AudioAnalyzer() = default;
    ~AudioAnalyzer();

    bool LoadAndComputeTimeline(const std::string& filePath, double videoDuration);

    const std::vector<AudioTrack>& GetTracks() const { return m_tracks; }
    float GetGlobalMaxPeak() const { return m_globalMaxPeak; }

    int GetTotalSeconds() const;
    const std::vector<float>& GetWaveform(int trackIndex) const;

    std::vector<float> GetWaveformAtTime(double currentTime, int trackIndex) const;
    std::vector<float> GetCombinedWaveformAtTime(double currentTime) const;

private:
    AVFormatContext* m_formatCtx = nullptr;
    std::vector<AudioTrack> m_tracks;
    float m_globalMaxPeak = 0.1f;
    double m_videoDuration = 0.0;

    SwrContext* m_swrCtx = nullptr;
    uint8_t* m_audioBuffer = nullptr;
    int m_audioBufferSize = 0;

    void Cleanup();
    void ExtractAudioTracks();
    void PreComputeTimeline();
    std::vector<float> ComputeWaveform(const float* samples, int sampleCount);
};