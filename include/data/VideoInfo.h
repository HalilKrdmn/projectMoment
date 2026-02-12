#pragma once

#include <filesystem>
#include <imgui.h>
#include <vector>

struct VideoInfo {
    std::filesystem::path filePath;
    std::string filePathString;
    std::string name{};


    double durationSec{};
    int frameRate{};

    int resolutionWidth{};
    int resolutionHeight{};
    int fileResolutionWidth{};
    int fileResolutionHeight{};

    std::string thumbnailPath{};
    mutable ImTextureID thumbnailId = reinterpret_cast<ImTextureID>(nullptr);

    std::string description{};
    std::string tagsStorage;

    double clipStartPoint{};
    double clipEndPoint{};

    long long recordingTimeMs{};
    long long lastEditTimeMs{};

    std::string audioCodec{};
    int audioBitrate{};
    int audioSampleRate{};
    std::string audioTrackNamesStr;
    std::vector<std::string> audioTrackNamesVector;

    std::string appVersion{};


    int64_t fileSize{};
    int64_t lastModified = 0;
    std::string codec{};
    int64_t bitrate{};
};