#pragma once

#include <filesystem>

#include <imgui.h>
#include <vector>

struct VideoInfo {
    std::string appVersion{};

    // File Information
    std::filesystem::path filePath;
    std::string filePathString;
    std::string name{};
    int64_t fileSize{};
    int64_t lastModified{};

    // Video Data
    double durationSec{};
    int frameRate{};
    int resolutionWidth{};
    int resolutionHeight{};

    // Application Logic
    mutable ImTextureID thumbnailId = reinterpret_cast<ImTextureID>(nullptr);
    std::string thumbnailPath{};
    bool isFavorite = false;

    // Time and Timeline Data
    double clipStartPoint{};
    double clipEndPoint{};
    long long recordingTimeMs{};
    long long lastEditTimeMs{};

    // Audio Track Names
    std::vector<std::string> audioTrackNames;
};