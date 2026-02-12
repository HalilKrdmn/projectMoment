#pragma once

#include <string>
#include <filesystem>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace fs = std::filesystem;

enum class ThumbnailStrategy {
    FIRST_FRAME,
    FRAME_AT_1SEC,
    FRAME_AT_10SEC,
    MIDDLE_FRAME,
    RANDOM_FRAME,
    SMART_FRAME
};

class ThumbnailService {
public:
    explicit ThumbnailService(const std::string& thumbFolder);

    std::string GenerateThumbnail(
        const std::string& videoPath,
        ThumbnailStrategy strategy = ThumbnailStrategy::FRAME_AT_1SEC,
        int thumbnailWidth = 320,
        int thumbnailHeight = 180
    ) const;

private:
    std::string thumbnailFolder;
    std::string GetThumbnailPath(const std::string &videoPath) const;

    static bool ExtractFrame(
        AVFormatContext* formatCtx,
        AVCodecContext* codecCtx,
        int videoStreamIndex,
        int64_t targetPts,
        const std::string& outputPath,
        int thumbnailWidth,
        int thumbnailHeight
    );

    static int64_t CalculateTargetPts(
        const AVFormatContext* formatCtx,
        int videoStreamIndex,
        ThumbnailStrategy strategy
    );

    static AVFrame* DecodeFrame(
        AVFormatContext* formatCtx,
        AVCodecContext* codecCtx,
        int videoStreamIndex,
        int64_t targetPts
    );

    static bool SaveFrameAsJPEG(
        AVFrame* frame,
        const std::string& outputPath,
        int width,
        int height
    );

};