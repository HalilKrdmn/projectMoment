#include "gui/utils/ThumbnailLoader.h"
#include "core/VideoInfo.h"
#include <GL/gl.h>
#include <filesystem>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

ImTextureID ThumbnailLoader::LoadFromFile(const char* filename) {
    if (!std::filesystem::exists(filename)) {
        std::cerr << "Thumbnail file not found: " << filename << std::endl;
        return 0;
    }

    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4); // RGBA

    if (!data) {
        std::cerr << "Failed to load thumbnail: " << filename << std::endl;
        std::cerr << "stbi error: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    std::cout << "✓ Loaded thumbnail: " << filename
              << " (" << width << "x" << height << ")" << std::endl;

    return (ImTextureID)(intptr_t)textureID;
}

void ThumbnailLoader::FreeTexture(ImTextureID texture) {
    if (texture) {
        GLuint textureID = (GLuint)(intptr_t)texture;
        glDeleteTextures(1, &textureID);
    }
}

void ThumbnailLoader::LoadThumbnails(std::vector<VideoInfo>& videos) {
    std::cout << "Loading thumbnails for " << videos.size() << " videos..." << std::endl;

    int loaded = 0;
    int failed = 0;

    for (auto& video : videos) {
        if (!video.thumbnailPath.empty()) {
            video.thumbnailId = LoadFromFile(video.thumbnailPath.c_str());

            if (video.thumbnailId) {
                loaded++;
            } else {
                failed++;
                std::cerr << "✗ Failed to load: " << video.thumbnailPath << std::endl;
            }
        } else {
            std::cout << "⊘ No thumbnail path for: " << video.name << std::endl;
        }
    }

    std::cout << "Thumbnail loading completed: "
              << loaded << " loaded, "
              << failed << " failed" << std::endl;
}

void ThumbnailLoader::FreeThumbnails(std::vector<VideoInfo>& videos) {
    std::cout << "Freeing thumbnails for " << videos.size() << " videos..." << std::endl;

    for (auto& video : videos) {
        if (video.thumbnailId) {
            FreeTexture(video.thumbnailId);
            video.thumbnailId = 0;
        }
    }

    std::cout << "All thumbnails freed" << std::endl;
}