#pragma once

#include <vector>

#include "imgui.h"

struct VideoInfo;

class ThumbnailLoader {
public:
    static ImTextureID LoadFromFile(const char* filename);
    static void FreeTexture(ImTextureID texture);
    static void LoadThumbnails(std::vector<VideoInfo>& videos);
    static void FreeThumbnails(std::vector<VideoInfo>& videos);

private:
    ThumbnailLoader() = delete;  // Static-only class
};