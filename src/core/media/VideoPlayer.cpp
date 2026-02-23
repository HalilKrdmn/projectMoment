#include "core/media/VideoPlayer.h"

#include <iostream>
#include <glad/glad.h>

VideoPlayer::~VideoPlayer() {
    Cleanup();
}

bool VideoPlayer::LoadVideo(const std::string& filePath) {
    Cleanup();

    if (avformat_open_input(&m_formatCtx, filePath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "[VideoPlayer] Failed to open: " << filePath << std::endl;
        return false;
    }

    if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
        std::cerr << "[VideoPlayer] Failed to find stream info" << std::endl;
        avformat_close_input(&m_formatCtx);
        return false;
    }

    m_videoStreamIndex = av_find_best_stream(m_formatCtx, AVMEDIA_TYPE_VIDEO,
                                             -1, -1, nullptr, 0);
    if (m_videoStreamIndex < 0) {
        std::cerr << "[VideoPlayer] No video stream found" << std::endl;
        avformat_close_input(&m_formatCtx);
        return false;
    }

    m_videoStream = m_formatCtx->streams[m_videoStreamIndex];

    const AVCodec* codec = avcodec_find_decoder(m_videoStream->codecpar->codec_id);
    if (!codec) {
        std::cerr << "[VideoPlayer] Codec not found" << std::endl;
        avformat_close_input(&m_formatCtx);
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        std::cerr << "[VideoPlayer] Failed to allocate codec context" << std::endl;
        avformat_close_input(&m_formatCtx);
        return false;
    }

    if (avcodec_parameters_to_context(m_codecCtx, m_videoStream->codecpar) < 0) {
        std::cerr << "[VideoPlayer] Failed to copy codec parameters" << std::endl;
        avcodec_free_context(&m_codecCtx);
        avformat_close_input(&m_formatCtx);
        return false;
    }

    // Fix: explicitly set color range so swscaler doesn't warn about deprecated pix_fmt
    if (m_codecCtx->color_range == AVCOL_RANGE_UNSPECIFIED)
        m_codecCtx->color_range = AVCOL_RANGE_MPEG;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        std::cerr << "[VideoPlayer] Failed to open codec" << std::endl;
        avcodec_free_context(&m_codecCtx);
        avformat_close_input(&m_formatCtx);
        return false;
    }

    m_width     = m_codecCtx->width;
    m_height    = m_codecCtx->height;
    m_duration  = (m_formatCtx->duration != AV_NOPTS_VALUE)
                ? (double)m_formatCtx->duration / AV_TIME_BASE
                : 0.0;
    m_frameRate = av_q2d(m_videoStream->r_frame_rate);
    if (m_frameRate <= 0.0) m_frameRate = 30.0; // safe fallback
    m_frameTime = 1.0 / m_frameRate;

    std::cout << "[VideoPlayer] Loaded: " << m_width << "x" << m_height
              << " @ " << m_frameRate << " fps, duration: " << m_duration << "s"
              << std::endl;

    m_frame    = av_frame_alloc();
    m_rgbFrame = av_frame_alloc();
    if (!m_frame || !m_rgbFrame) {
        std::cerr << "[VideoPlayer] Failed to allocate frames" << std::endl;
        Cleanup();
        return false;
    }

    // Use sws_getContext with explicit pixel format; avoids the deprecated-pix-fmt warning
    // Map the codec pixel format — fall back to yuv420p if unknown
    AVPixelFormat srcFmt = m_codecCtx->pix_fmt;
    if (srcFmt == AV_PIX_FMT_NONE) srcFmt = AV_PIX_FMT_YUV420P;

    m_swsCtx = sws_getContext(
        m_width, m_height, srcFmt,
        m_width, m_height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!m_swsCtx) {
        std::cerr << "[VideoPlayer] Failed to create SWS context" << std::endl;
        Cleanup();
        return false;
    }

    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, m_width, m_height, 1);
    m_buffer = static_cast<uint8_t*>(av_malloc(bufferSize));
    if (!m_buffer) {
        std::cerr << "[VideoPlayer] Failed to allocate RGB buffer" << std::endl;
        Cleanup();
        return false;
    }

    av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize,
                         m_buffer, AV_PIX_FMT_RGB24, m_width, m_height, 1);

    CreateTexture();

    m_isLoaded   = true;
    m_currentTime = 0.0;
    return true;
}

void VideoPlayer::CreateTexture() {
    if (m_textureId) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }

    glGenTextures(1, &m_textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "[VideoPlayer] Created texture " << m_textureId << std::endl;
}

void VideoPlayer::Update(float deltaTime) {
    if (!m_isLoaded || !m_isPlaying) return;

    m_frameAccumulator += deltaTime;

    while (m_frameAccumulator >= m_frameTime && m_isPlaying) {
        DecodeFrame();
        m_frameAccumulator -= m_frameTime;
        m_currentTime      += m_frameTime;

        if (m_currentTime >= m_duration) {
            m_currentTime = m_duration;
            m_isPlaying   = false;
        }
    }
}

void VideoPlayer::DecodeFrame() {
    if (!m_isLoaded) return;

    // ✅ Use heap-allocated packet (av_init_packet is removed in FFmpeg 5+)
    AVPacket* packet = av_packet_alloc();
    if (!packet) return;

    bool decoded = false;

    while (!decoded && av_read_frame(m_formatCtx, packet) >= 0) {
        if (packet->stream_index == m_videoStreamIndex) {
            int ret = avcodec_send_packet(m_codecCtx, packet);
            if (ret >= 0) {
                ret = avcodec_receive_frame(m_codecCtx, m_frame);
                if (ret == 0) {
                    // Convert to RGB
                    sws_scale(m_swsCtx,
                              (const uint8_t* const*)m_frame->data,
                              m_frame->linesize, 0, m_codecCtx->height,
                              m_rgbFrame->data, m_rgbFrame->linesize);

                    // Upload to GPU texture
                    glBindTexture(GL_TEXTURE_2D, m_textureId);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height,
                                   GL_RGB, GL_UNSIGNED_BYTE, m_buffer);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    decoded = true;
                }
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
}

void VideoPlayer::Play() {
    if (m_isLoaded) {
        m_isPlaying = true;
        std::cout << "[VideoPlayer] Playing" << std::endl;
    }
}

void VideoPlayer::Pause() {
    m_isPlaying = false;
    std::cout << "[VideoPlayer] Paused at " << m_currentTime << "s" << std::endl;
}

void VideoPlayer::Stop() {
    m_isPlaying        = false;
    m_currentTime      = 0.0;
    m_frameAccumulator = 0.0;

    if (m_formatCtx)
        av_seek_frame(m_formatCtx, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);

    std::cout << "[VideoPlayer] Stopped" << std::endl;
}

void VideoPlayer::Seek(double seconds) {
    if (!m_isLoaded) return;

    seconds = std::max(0.0, std::min(seconds, m_duration));

    int64_t timestamp = static_cast<int64_t>(
        seconds / av_q2d(m_videoStream->time_base));

    if (av_seek_frame(m_formatCtx, m_videoStreamIndex,
                      timestamp, AVSEEK_FLAG_BACKWARD) >= 0) {
        avcodec_flush_buffers(m_codecCtx);
        m_currentTime      = seconds;
        m_frameAccumulator = 0.0;
        std::cout << "[VideoPlayer] Seeked to " << seconds << "s" << std::endl;
    }
}

void VideoPlayer::Cleanup() {
    m_isPlaying = false;
    m_isLoaded  = false;

    if (m_textureId) {
        glDeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
    if (m_buffer) {
        av_free(m_buffer);
        m_buffer = nullptr;
    }
    if (m_swsCtx) {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_rgbFrame) {
        av_frame_free(&m_rgbFrame);
        m_rgbFrame = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
        m_codecCtx = nullptr;
    }
    if (m_formatCtx) {
        avformat_close_input(&m_formatCtx);
        m_formatCtx = nullptr;
    }
}