#include "core/media/AudioAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

AudioAnalyzer::~AudioAnalyzer() {
    Cleanup();
}

bool AudioAnalyzer::LoadAndComputeTimeline(const std::string& filePath,
                                           double videoDuration) {
    Cleanup();
    m_videoDuration = videoDuration;

    if (avformat_open_input(&m_formatCtx, filePath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "[AudioAnalyzer] Failed to open: " << filePath << std::endl;
        return false;
    }
    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        std::cerr << "[AudioAnalyzer] Failed to find stream info" << std::endl;
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
        return false;
    }

    ExtractAudioTracks();
    if (m_ctx.empty()) return false;

    std::cout << "[AudioAnalyzer] Computing timeline for "
              << videoDuration << "s, " << m_ctx.size() << " track(s)..." << std::endl;
    PreComputeTimeline();
    std::cout << "[AudioAnalyzer] Done. Global peak: " << m_globalMaxPeak << std::endl;
    return true;
}

void AudioAnalyzer::ExtractAudioTracks() {
    m_ctx.clear();
    m_tracks.clear();
    m_globalMaxPeak = 0.001f;

    const int totalSeconds = (int)std::ceil(m_videoDuration);

    for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
        AVStream* stream = m_formatCtx->streams[i];
        if (stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;

        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) continue;

        AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) continue;

        if (avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0) {
            avcodec_free_context(&codecCtx);
            continue;
        }
        if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
            avcodec_free_context(&codecCtx);
            continue;
        }

        // ── Name ────────────────────────────────────────────────────────────
        std::string trackName;
        {
            AVDictionaryEntry* e = av_dict_get(stream->metadata, "title", nullptr, 0);
            if (e && e->value && e->value[0] != '\0') {
                trackName = e->value;
            } else {
                const int idx = (int)m_ctx.size();
                if      (idx == 0) trackName = "Main Audio";
                else if (idx == 1) trackName = "Game Audio";
                else if (idx == 2) trackName = "Discord Voice";
                else if (idx == 3) trackName = "Background Music";
                else               trackName = "Audio Track " + std::to_string(idx + 1);
            }
        }

        // ── SwrContext ───────────────────────────────────────────────────────
        SwrContext* swrCtx = nullptr;
        {
            if (codecCtx->ch_layout.nb_channels <= 0)
                av_channel_layout_default(&codecCtx->ch_layout, 2);

            AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_MONO;
            int ret = swr_alloc_set_opts2(
                &swrCtx,
                &outLayout,           AV_SAMPLE_FMT_FLT, codecCtx->sample_rate,
                &codecCtx->ch_layout, (AVSampleFormat)codecCtx->sample_fmt,
                codecCtx->sample_rate, 0, nullptr);

            if (ret < 0 || swr_init(swrCtx) < 0) {
                if (swrCtx) swr_free(&swrCtx);
                swrCtx = nullptr;
                std::cerr << "[AudioAnalyzer] swr_init failed: " << trackName << std::endl;
            }
        }

        // ── Push internal ctx ────────────────────────────────────────────────
        {
            TrackCtx tc;
            tc.streamIndex = (int)i;
            tc.codecCtx    = codecCtx;
            tc.swrCtx      = swrCtx;
            tc.rawRms.assign(totalSeconds, 0.0f);
            tc.maxPeak     = 0.001f;
            m_ctx.push_back(std::move(tc));
        }

        // ── Push public WaveformTrack ────────────────────────────────────────
        {
            WaveformTrack wt;
            wt.streamIndex  = (int)i;
            wt.name         = trackName;          // plain copy — no move
            wt.waveform.assign(totalSeconds, 0.0f);
            wt.maxPeakLevel = 0.001f;
            m_tracks.push_back(std::move(wt));
        }

        std::cout << "[AudioAnalyzer] Track " << m_ctx.size()
                  << ": " << trackName
                  << " (" << codecCtx->sample_rate << " Hz, "
                  << codecCtx->ch_layout.nb_channels << " ch)"
                  << (swrCtx ? "" : " [NO SWR]") << std::endl;
    }
}

void AudioAnalyzer::PreComputeTimeline() {
    av_seek_frame(m_formatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);

    AVPacket* packet = av_packet_alloc();
    if (!packet) return;

    const int totalSeconds = (int)std::ceil(m_videoDuration);
    std::vector<float> convertBuf(192000);
    int frameCount = 0;

    while (av_read_frame(m_formatCtx, packet) >= 0) {
        for (size_t ti = 0; ti < m_ctx.size(); ti++) {
            TrackCtx& tc = m_ctx[ti];
            if (packet->stream_index != tc.streamIndex) continue;
            if (!tc.swrCtx) continue;

            if (avcodec_send_packet(tc.codecCtx, packet) < 0) break;

            AVFrame* frame = av_frame_alloc();
            if (!frame) break;

            while (avcodec_receive_frame(tc.codecCtx, frame) == 0) {
                if (frame->nb_samples <= 0) continue;

                double frameTime = 0.0;
                if (frame->pts != AV_NOPTS_VALUE)
                    frameTime = frame->pts *
                        av_q2d(m_formatCtx->streams[tc.streamIndex]->time_base);

                const int sec = (int)std::floor(frameTime);
                if (sec < 0 || sec >= totalSeconds) continue;

                if (frame->nb_samples > (int)convertBuf.size())
                    convertBuf.resize(frame->nb_samples);

                uint8_t* outPtr = reinterpret_cast<uint8_t*>(convertBuf.data());
                int nb = swr_convert(tc.swrCtx,
                                     &outPtr, frame->nb_samples,
                                     (const uint8_t**)frame->data, frame->nb_samples);
                if (nb <= 0) continue;

                float sumSq = 0.0f;
                for (int s = 0; s < nb; s++) {
                    float v = convertBuf[s];
                    sumSq += v * v;
                    float av = std::fabs(v);
                    if (av > tc.maxPeak)      tc.maxPeak      = av;
                    if (av > m_globalMaxPeak) m_globalMaxPeak = av;
                }
                float rms = std::sqrt(sumSq / nb);
                if (rms > tc.rawRms[sec]) tc.rawRms[sec] = rms;
            }
            av_frame_free(&frame);
        }
        av_packet_unref(packet);
        frameCount++;
    }
    av_packet_free(&packet);

    // Pass 2: normalize into public waveform
    for (size_t ti = 0; ti < m_ctx.size(); ti++) {
        TrackCtx&     tc = m_ctx[ti];
        WaveformTrack& wt = m_tracks[ti];
        wt.maxPeakLevel   = tc.maxPeak;
        const float ref   = std::max(0.001f, tc.maxPeak);
        for (int s = 0; s < totalSeconds; s++)
            wt.waveform[s] = std::min(1.0f, (tc.rawRms[s] / ref) * 2.5f);
    }

    std::cout << "[AudioAnalyzer] " << frameCount
              << " frames processed across " << m_ctx.size() << " track(s)" << std::endl;
}

int AudioAnalyzer::GetTotalSeconds() const {
    return m_tracks.empty() ? 0 : (int)m_tracks[0].waveform.size();
}

const std::vector<float>& AudioAnalyzer::GetWaveform(int idx) const {
    static const std::vector<float> empty;
    if (idx < 0 || idx >= (int)m_tracks.size()) return empty;
    return m_tracks[idx].waveform;
}

bool AudioAnalyzer::IsTrackSilent(int idx, float threshold) const {
    if (idx < 0 || idx >= (int)m_tracks.size()) return true;
    return m_tracks[idx].maxPeakLevel < threshold;
}

void AudioAnalyzer::Cleanup() {
    for (auto& tc : m_ctx) {
        if (tc.swrCtx)  { swr_free(&tc.swrCtx);              tc.swrCtx  = nullptr; }
        if (tc.codecCtx){ avcodec_free_context(&tc.codecCtx); tc.codecCtx = nullptr; }
    }
    m_ctx.clear();
    m_tracks.clear();

    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }
    m_globalMaxPeak = 0.001f;
}