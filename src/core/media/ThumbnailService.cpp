#include "core/media/ThumbnailService.h"

#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <vector>

ThumbnailService::ThumbnailService(const std::string& thumbFolder)
    : thumbnailFolder(thumbFolder) {
}

std::string ThumbnailService::GenerateThumbnail(
    const std::string& videoPath,
    const ThumbnailStrategy strategy,
    const int thumbnailWidth,
    const int thumbnailHeight) const {

    // Thumbnail path create
    std::string thumbnailPath = GetThumbnailPath(videoPath);

    if (fs::exists(thumbnailPath)) {
        return thumbnailPath;
    }

    // Open video folder
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr) != 0) {
        return "";
    }

    // Stream info
    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        avformat_close_input(&formatCtx);
        return "";
    }

    // Find video stream
    int videoStreamIndex = -1;
    const AVCodecParameters* codecParams = nullptr;

    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            codecParams = formatCtx->streams[i]->codecpar;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        avformat_close_input(&formatCtx);
        return "";
    }

    // Open codec
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    if (!codec) {
        avformat_close_input(&formatCtx);
        return "";
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecParams);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return "";
    }

    const int64_t targetPts = CalculateTargetPts(formatCtx, videoStreamIndex, strategy);

    const bool success = ExtractFrame(
        formatCtx, codecCtx, videoStreamIndex,
        targetPts, thumbnailPath,
        thumbnailWidth, thumbnailHeight
    );

    // Cleaning
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);

    return success ? thumbnailPath : "";
}

std::string ThumbnailService::GetThumbnailPath(const std::string& videoPath) const {
    const fs::path video(videoPath);
    const std::string filename = video.stem().string();

    // Thumbnail path: thumbnails/video_name.jpg
    const fs::path thumbPath = fs::path(thumbnailFolder) / (filename + ".jpg");
    return thumbPath.string();
}

int64_t ThumbnailService::CalculateTargetPts(
    const AVFormatContext* formatCtx,
    const int videoStreamIndex,
    const ThumbnailStrategy strategy) {

    const AVStream* videoStream = formatCtx->streams[videoStreamIndex];
    int64_t duration = videoStream->duration;

    if (duration == AV_NOPTS_VALUE) {
        duration = formatCtx->duration * videoStream->time_base.den /
                   (AV_TIME_BASE * videoStream->time_base.num);
    }

    int64_t targetPts = 0;

    switch (strategy) {
        case ThumbnailStrategy::FIRST_FRAME:
            targetPts = 0;
            break;

        case ThumbnailStrategy::FRAME_AT_1SEC:
            targetPts = av_rescale_q(
                1 * AV_TIME_BASE,
                {1, AV_TIME_BASE},
                videoStream->time_base
            );
            break;

        case ThumbnailStrategy::FRAME_AT_10SEC:
            targetPts = av_rescale_q(
                10 * AV_TIME_BASE,
                {1, AV_TIME_BASE},
                videoStream->time_base
            );
            break;

        case ThumbnailStrategy::MIDDLE_FRAME:
            targetPts = duration / 2;
            break;

        case ThumbnailStrategy::RANDOM_FRAME:
            srand(time(nullptr));
            targetPts = (duration * 4 / 5) * rand() / RAND_MAX;
            break;

        case ThumbnailStrategy::SMART_FRAME:
            targetPts = av_rescale_q(
                1 * AV_TIME_BASE,
                {1, AV_TIME_BASE},
                videoStream->time_base
            );
            break;
    }

    if (targetPts >= duration) {
        targetPts = duration / 2;
    }

    return targetPts;
}

bool ThumbnailService::ExtractFrame(
    AVFormatContext* formatCtx,
    AVCodecContext* codecCtx,
    const int videoStreamIndex,
    const int64_t targetPts,
    const std::string& outputPath,
    const int thumbnailWidth,
    const int thumbnailHeight) {

    av_seek_frame(formatCtx, videoStreamIndex, targetPts, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecCtx);

    AVFrame* frame = DecodeFrame(formatCtx, codecCtx, videoStreamIndex, targetPts);
    if (!frame) {
        return false;
    }

    const bool success = SaveFrameAsJPEG(frame, outputPath, thumbnailWidth, thumbnailHeight);

    av_frame_free(&frame);
    return success;
}

AVFrame* ThumbnailService::DecodeFrame(
    AVFormatContext* formatCtx,
    AVCodecContext* codecCtx,
    const int videoStreamIndex,
    const int64_t targetPts) {

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    AVFrame* targetFrame = nullptr;

    while (av_read_frame(formatCtx, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecCtx, packet) == 0) {
                while (avcodec_receive_frame(codecCtx, frame) == 0) {
                    if (frame->pts >= targetPts) {
                        targetFrame = av_frame_clone(frame);
                        av_packet_unref(packet);
                        goto cleanup;
                    }
                }
            }
        }
        av_packet_unref(packet);
    }

cleanup:
    av_packet_free(&packet);
    av_frame_free(&frame);

    return targetFrame;
}

bool ThumbnailService::SaveFrameAsJPEG(
    AVFrame* frame,
    const std::string& outputPath,
    const int width,
    const int height) {

    SwsContext* swsCtx = sws_getContext(
        frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!swsCtx) {
        return false;
    }

    AVFrame* rgbFrame = av_frame_alloc();
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = width;
    rgbFrame->height = height;

    av_frame_get_buffer(rgbFrame, 0);

    sws_scale(
        swsCtx,
        frame->data, frame->linesize, 0, frame->height,
        rgbFrame->data, rgbFrame->linesize
    );

    // JPEG encoder
    const AVCodec* jpegCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    AVCodecContext* jpegCtx = avcodec_alloc_context3(jpegCodec);

    jpegCtx->width = width;
    jpegCtx->height = height;
    jpegCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    jpegCtx->time_base = {1, 25};
    jpegCtx->qmin = jpegCtx->qmax = 2;

    avcodec_open2(jpegCtx, jpegCodec, nullptr);

    SwsContext* swsCtx2 = sws_getContext(
        width, height, AV_PIX_FMT_RGB24,
        width, height, AV_PIX_FMT_YUVJ420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    AVFrame* yuvFrame = av_frame_alloc();
    yuvFrame->format = AV_PIX_FMT_YUVJ420P;
    yuvFrame->width = width;
    yuvFrame->height = height;
    av_frame_get_buffer(yuvFrame, 0);

    sws_scale(
        swsCtx2,
        rgbFrame->data, rgbFrame->linesize, 0, height,
        yuvFrame->data, yuvFrame->linesize
    );

    AVPacket* packet = av_packet_alloc();
    avcodec_send_frame(jpegCtx, yuvFrame);
    avcodec_receive_packet(jpegCtx, packet);

    if (FILE* file = fopen(outputPath.c_str(), "wb")) {
        fwrite(packet->data, 1, packet->size, file);
        fclose(file);
    }

    av_packet_free(&packet);
    av_frame_free(&yuvFrame);
    av_frame_free(&rgbFrame);
    avcodec_free_context(&jpegCtx);
    sws_freeContext(swsCtx);
    sws_freeContext(swsCtx2);

    return true;
}
