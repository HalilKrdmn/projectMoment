#include "core/media/AudioAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

AudioAnalyzer::~AudioAnalyzer() {
    Cleanup();
}

bool AudioAnalyzer::LoadAndComputeTimeline(const std::string& filePath, double videoDuration) {
    Cleanup();

    m_formatCtx = nullptr;
    m_videoDuration = videoDuration;

    if (avformat_open_input(&m_formatCtx, filePath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "[AudioAnalyzer] Failed to open: " << filePath << std::endl;
        return false;
    }

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        std::cerr << "[AudioAnalyzer] Failed to find stream info" << std::endl;
        avformat_close_input(&m_formatCtx);
        return false;
    }

    ExtractAudioTracks();

    if (m_tracks.empty()) return false;

    std::cout << "[AudioAnalyzer] Pre-computing timeline for " << videoDuration << " seconds..." << std::endl;
    PreComputeTimeline();

    std::cout << "[AudioAnalyzer] Timeline computation complete!" << std::endl;
    std::cout << "[AudioAnalyzer] Global max peak: " << m_globalMaxPeak << std::endl;

    return true;
}

void AudioAnalyzer::ExtractAudioTracks() {
    m_tracks.clear();
    m_globalMaxPeak = 0.1f;

    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        AVStream* stream = m_formatCtx->streams[i];

        if (stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;

        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) continue;

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) continue;

        avcodec_parameters_to_context(codecCtx, stream->codecpar);

        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            avcodec_free_context(&codecCtx);
            continue;
        }

        AudioTrack track;
        track.streamIndex = i;
        track.codecCtx = codecCtx;

        AVDictionary* metadata = stream->metadata;
        AVDictionaryEntry* entry = av_dict_get(metadata, "title", nullptr, 0);
        if (entry && entry->value) {
            track.name = entry->value;
        } else {
            int audioIndex = m_tracks.size();
            if (audioIndex == 0) track.name = "Main Audio";
            else if (audioIndex == 1) track.name = "Game Audio";
            else if (audioIndex == 2) track.name = "Discord Voice";
            else if (audioIndex == 3) track.name = "Background Music";
            else track.name = "Audio Track " + std::to_string(audioIndex + 1);
        }

        int totalSeconds = (int)std::ceil(m_videoDuration);
        track.waveform.resize(totalSeconds, 0.0f);
        track.maxPeakLevel = 0.1f;

        m_tracks.push_back(track);

        std::cout << "[AudioAnalyzer] Track " << m_tracks.size() << ": "
                  << track.name << " (" << codecCtx->sample_rate << "Hz)" << std::endl;
    }
}

void AudioAnalyzer::PreComputeTimeline() {
    av_seek_frame(m_formatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);

    AVPacket* packet = av_packet_alloc();
    if (!packet) return;

    int frameCount = 0;
    int totalSeconds = (int)std::ceil(m_videoDuration);

    while (av_read_frame(m_formatCtx, packet) >= 0) {
        for (size_t trackIdx = 0; trackIdx < m_tracks.size(); trackIdx++) {
            if (packet->stream_index != m_tracks[trackIdx].streamIndex) continue;

            int response = avcodec_send_packet(m_tracks[trackIdx].codecCtx, packet);
            if (response < 0) break;

            AVFrame* frame = av_frame_alloc();
            if (avcodec_receive_frame(m_tracks[trackIdx].codecCtx, frame) == 0) {
                if (frame->nb_samples > 0) {
                    double frameTime = 0.0;
                    if (frame->pts != AV_NOPTS_VALUE) {
                        frameTime = frame->pts * av_q2d(m_tracks[trackIdx].codecCtx->time_base);
                    }

                    int secondIndex = (int)std::floor(frameTime);
                    if (secondIndex >= 0 && secondIndex < totalSeconds) {
                        SwrContext* swrCtx = swr_alloc();
                        if (swrCtx) {
                            AVChannelLayout inLayout = frame->ch_layout;
                            if (inLayout.nb_channels == 0) {
                                av_channel_layout_default(&inLayout, frame->nb_samples);
                            }

                            AVChannelLayout outLayout;
                            av_channel_layout_from_mask(&outLayout, AV_CH_LAYOUT_MONO);

                            swr_alloc_set_opts2(&swrCtx,
                                                &outLayout,
                                                AV_SAMPLE_FMT_FLT,
                                                frame->sample_rate,
                                                &inLayout,
                                                (AVSampleFormat)frame->format,
                                                frame->sample_rate,
                                                0, nullptr);

                            if (swr_init(swrCtx) >= 0) {
                                uint8_t* out[1];
                                out[0] = static_cast<uint8_t *>(av_malloc(frame->nb_samples * sizeof(float)));

                                int nbSamples = swr_convert(swrCtx, out, frame->nb_samples,
                                                           frame->data, frame->nb_samples);

                                if (nbSamples > 0) {
                                    float* samples = (float*)out[0];

                                    float rms = 0.0f;
                                    for (int i = 0; i < nbSamples; i++) {
                                        float sample = samples[i];
                                        rms += sample * sample;
                                    }
                                    rms = std::sqrt(rms / nbSamples);

                                    float normalized = rms / std::max(0.1f, m_globalMaxPeak);
                                    float amplified = std::min(1.0f, normalized * 3.0f);

                                    m_tracks[trackIdx].waveform[secondIndex] = amplified;

                                    for (int i = 0; i < nbSamples; i++) {
                                        float val = std::fabs(samples[i]);
                                        if (val > m_tracks[trackIdx].maxPeakLevel) {
                                            m_tracks[trackIdx].maxPeakLevel = val;
                                        }
                                        if (val > m_globalMaxPeak) {
                                            m_globalMaxPeak = val;
                                        }
                                    }
                                }

                                av_freep(&out[0]);
                            }

                            swr_free(&swrCtx);
                            av_channel_layout_uninit(&outLayout);
                        }
                    }
                }
            }
            av_frame_free(&frame);
        }

        av_packet_unref(packet);
        frameCount++;
    }

    av_packet_free(&packet);

    std::cout << "[AudioAnalyzer] Processed " << frameCount << " frames" << std::endl;
    std::cout << "[AudioAnalyzer] Timeline computed for " << totalSeconds << " seconds" << std::endl;
}

std::vector<float> AudioAnalyzer::ComputeWaveform(const float* samples, int sampleCount) {
    std::vector<float> waveform(64, 0.0f);
    if (sampleCount == 0) return waveform;

    int samplesPerBar = sampleCount / 64;
    if (samplesPerBar < 1) samplesPerBar = 1;

    for (int bar = 0; bar < 64; bar++) {
        float rms = 0.0f;
        int count = 0;

        for (int i = bar * samplesPerBar; i < (bar + 1) * samplesPerBar && i < sampleCount; i++) {
            float sample = samples[i];
            rms += sample * sample;
            count++;
        }

        if (count > 0) {
            rms = std::sqrt(rms / count);
        }

        waveform[bar] = rms * 2.0f;
    }

    return waveform;
}

int AudioAnalyzer::GetTotalSeconds() const {
    if (m_tracks.empty()) return 0;
    return (int)m_tracks[0].waveform.size();
}

const std::vector<float>& AudioAnalyzer::GetWaveform(int trackIndex) const {
    static const std::vector<float> empty;
    if (trackIndex < 0 || trackIndex >= (int)m_tracks.size()) {
        return empty;
    }
    return m_tracks[trackIndex].waveform;
}

std::vector<float> AudioAnalyzer::GetWaveformAtTime(double currentTime, int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= (int)m_tracks.size()) {
        return std::vector<float>(64, 0.0f);
    }

    int secondIndex = (int)std::floor(currentTime);
    const auto& waveform = m_tracks[trackIndex].waveform;

    if (secondIndex < 0 || secondIndex >= (int)waveform.size()) {
        return std::vector<float>(64, 0.0f);
    }

    std::vector<float> bars(64, waveform[secondIndex]);
    return bars;
}

std::vector<float> AudioAnalyzer::GetCombinedWaveformAtTime(double currentTime) const {
    std::vector<float> combined(64, 0.0f);

    if (m_tracks.empty()) return combined;

    for (size_t i = 0; i < m_tracks.size(); i++) {
        auto waveform = GetWaveformAtTime(currentTime, i);
        for (size_t j = 0; j < waveform.size(); j++) {
            combined[j] += waveform[j];
        }
    }

    float scale = 1.0f / m_tracks.size();
    for (auto& val : combined) {
        val = std::min(1.0f, val * scale);
    }

    return combined;
}

void AudioAnalyzer::Cleanup() {
    for (auto& track : m_tracks) {
        if (track.codecCtx) {
            avcodec_free_context(&track.codecCtx);
            track.codecCtx = nullptr;
        }
    }
    m_tracks.clear();

    if (m_swrCtx) {
        swr_free(&m_swrCtx);
        m_swrCtx = nullptr;
    }

    if (m_audioBuffer) {
        av_free(m_audioBuffer);
        m_audioBuffer = nullptr;
    }

    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }
}