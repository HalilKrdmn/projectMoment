#include "core/library/VideoLibrary.h"

#include <algorithm>

#include "core/library/VideoDatabase.h"
#include "core/media/ThumbnailService.h"
#include "core/media/MetadataEmbedder.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace logs{
    void LogInfo(const std::string& message) {
        std::cout << "[VideoLibrary] " << message << std::endl;
    }

    void LogWarning(const std::string& message) {
        std::cerr << "[VideoLibrary] WARNING: " << message << std::endl;
    }

    void LogError(const std::string& message) {
        std::cerr << "[VideoLibrary] ERROR: " << message << std::endl;
    }
}


VideoLibrary::VideoLibrary(const ProjectPaths& paths)
    : m_paths(paths) {

    EnsureDirectoriesExist();

    m_database = std::make_unique<VideoDatabase>(m_paths.dbPath.string());
    m_thumbnailService = std::make_unique<ThumbnailService>(m_paths.thumbFolder.string());
    m_metadataEmbedder = std::make_unique<MetadataEmbedder>();

    logs::LogInfo("VideoLibrary initialized successfully");
}

VideoLibrary::~VideoLibrary() {
    logs::LogInfo("VideoLibrary shutting down");
}

bool VideoLibrary::IsVideoFile(const fs::path& filePath) {
    // Check if file has an extension
    if (!filePath.has_extension()) {
        return false;
    }

    // Get the extension
    std::string ext = filePath.extension().string();

    // Extension should not be empty
    if (ext.empty()) {
        return false;
    }

    // Convert to lowercase for case-insensitive comparison
    std::transform(ext.begin(), ext.end(), ext.begin(),
                  [](unsigned char c) { return std::tolower(c); });

    // Check if extension matches any supported format
    return std::any_of(SUPPORTED_VIDEO_FORMATS.begin(),
                      SUPPORTED_VIDEO_FORMATS.end(),
                      [&ext](std::string_view fmt) {
                          return ext == fmt;
                      });
}


// QUERY OPERATIONS
std::vector<VideoInfo> VideoLibrary::GetAllVideos() const {
    return m_database->GetAllVideos();
}

std::optional<std::reference_wrapper<const VideoInfo>> VideoLibrary::GetVideo(const std::string& filePath) const {
    if (const auto* video = m_database->GetMetadata(filePath)) {
        return std::cref(*video);
    }
    return std::nullopt;
}

std::vector<VideoInfo> VideoLibrary::SearchByName(const std::string& query) const {
    return m_database->SearchByName(query);
}

std::vector<VideoInfo> VideoLibrary::SearchByTag(const std::string& tag) const {
    return m_database->GetVideosByTag(tag);
}

std::vector<VideoInfo> VideoLibrary::FilterByDuration(double minSec, double maxSec) const {
    auto allVideos = m_database->GetAllVideos();
    std::vector<VideoInfo> filtered;

    std::ranges::copy_if(allVideos,
                         std::back_inserter(filtered),
                         [minSec, maxSec](const VideoInfo& video) {
                             return video.durationSec >= minSec && video.durationSec <= maxSec;
                         });

    return filtered;
}

std::vector<VideoInfo> VideoLibrary::FilterByResolution(int minWidth, int minHeight) const {
    auto allVideos = m_database->GetAllVideos();
    std::vector<VideoInfo> filtered;

    std::ranges::copy_if(allVideos,
                         std::back_inserter(filtered),
                         [minWidth, minHeight](const VideoInfo& video) {
                             return video.resolutionWidth >= minWidth &&
                                    video.resolutionHeight >= minHeight;
                         });

    return filtered;
}

// VIDEO OPERATIONS
VideoInfo VideoLibrary::LoadVideo(const std::string& videoPath) {
    logs::LogInfo("Loading video: " + videoPath);

    try {
        // 1. Check cache first (fastest)
        if (auto* cached = m_database->GetMetadata(videoPath)) {
            logs::LogInfo("  Found in cache");
            return *cached;
        }

        // 2. Try embedded metadata (medium speed)
        VideoInfo info;
        if (m_metadataEmbedder && m_metadataEmbedder->HasEmbeddedMetadata(videoPath)) {
            logs::LogInfo("  Reading embedded metadata");
            if (m_metadataEmbedder->ReadMetadataFromVideo(videoPath, info)) {
                m_database->SaveMetadata(info);
                return info;
            }
        }

        // 3. Scan with FFmpeg (slowest)
        logs::LogInfo("  Scanning with FFmpeg");
        info = ScanVideoFile(videoPath);

        if (info.fileSize == 0) {
            logs::LogWarning("Could not scan video file: " + videoPath);
            return info;
        }

        // 4. Generate thumbnail
        if (auto thumbPath = GenerateThumbnail(videoPath)) {
            info.thumbnailPath = thumbPath.value();
        }

        // 5. Embed metadata
        if (m_metadataEmbedder) {
            m_metadataEmbedder->WriteMetadataToVideo(videoPath, info);
        }

        // 6. Save to database
        m_database->SaveMetadata(info);

        return info;

    } catch (const std::exception& e) {
        logs::LogError("Failed to load video: " + std::string(e.what()));
        return VideoInfo{};
    }
}

bool VideoLibrary::SaveVideo(const VideoInfo& info) {
    try {
        m_database->SaveMetadata(info);
        return true;
    } catch (const std::exception& e) {
        logs::LogError("Failed to save video: " + std::string(e.what()));
        return false;
    }
}

bool VideoLibrary::UpdateVideo(const VideoInfo& info, bool updateVideoFile) {
    try {
        m_database->SaveMetadata(info);

        if (updateVideoFile && m_metadataEmbedder) {
            return m_metadataEmbedder->WriteMetadataToVideo(info.filePathString, info);
        }

        return true;

    } catch (const std::exception& e) {
        logs::LogError("Failed to update video: " + std::string(e.what()));
        return false;
    }
}

// bool VideoLibrary::DeleteVideo(const std::string& filePath, bool deleteFromDisk) {
//     try {
//         if (!m_database) {
//             return false;
//         }
//
//         // Remove from database
//         // Implement VideoDatabase::DeleteMetadata()
//         logs::LogWarning("DeleteVideo: Database deletion not yet implemented");
//
//         // Optionally delete file from disk
//         if (deleteFromDisk && fs::exists(filePath)) {
//             fs::remove(filePath);
//             logs::LogInfo("Deleted video file: " + filePath);
//         }
//
//         return true;
//
//     } catch (const std::exception& e) {
//         logs::LogError("Failed to delete video: " + std::string(e.what()));
//         return false;
//     }
// }

void VideoLibrary::RegenerateMissingThumbnails() {
    logs::LogInfo("Regenerating missing thumbnails...");

    try {
        auto allVideos = m_database->GetAllVideos();
        size_t regenerated = 0;

        for (const auto& video : allVideos) {
            if (video.thumbnailPath.empty() || !fs::exists(video.thumbnailPath)) {
                if (auto thumbPath = GenerateThumbnail(video.filePathString)) {
                    VideoInfo updated = video;
                    updated.thumbnailPath = thumbPath.value();
                    m_database->SaveMetadata(updated);
                    regenerated++;
                }
            }
        }

        logs::LogInfo("Regenerated " + std::to_string(regenerated) + " thumbnails");

    } catch (const std::exception& e) {
        logs::LogError("Failed to regenerate thumbnails: " + std::string(e.what()));
    }
}


void VideoLibrary::SyncWithVideoFiles() const {
    logs::LogInfo("Syncing with video files...");

    try {
        auto allVideos = m_database->GetAllVideos();
        size_t synced = 0;

        for (const auto& video : allVideos) {
            if (!m_metadataEmbedder) {
                continue;
            }

            VideoInfo videoFileData;
            if (m_metadataEmbedder->ReadMetadataFromVideo(video.filePathString, videoFileData)) {
                if (videoFileData.lastEditTimeMs > video.lastEditTimeMs) {
                    m_database->SaveMetadata(videoFileData);
                    synced++;
                }
            }
        }

        logs::LogInfo("Synced " + std::to_string(synced) + " videos");

    } catch (const std::exception& e) {
        logs::LogError("Failed to sync with video files: " + std::string(e.what()));
    }
}

VideoLibrary::Statistics VideoLibrary::GetStatistics() const {
    Statistics stats;

    try {
        const auto& allVideos = m_database->GetAllVideos();
        stats.totalVideos = allVideos.size();

        for (const auto& video : allVideos) {
            stats.totalDurationSec += video.durationSec;

            // Avoid disk I/O if size is cached
            if (video.fileSize > 0) {
                stats.totalSizeBytes += video.fileSize;
            } else if (fs::exists(video.filePath)) {
                stats.totalSizeBytes += fs::file_size(video.filePath);
            }

            if (!video.thumbnailPath.empty() && fs::exists(video.thumbnailPath)) {
                stats.videosWithThumbnails++;
            }

            // Avoid service call if metadata already known
            if (m_metadataEmbedder && m_metadataEmbedder->HasEmbeddedMetadata(video.filePathString)) {
                stats.videosWithEmbeddedMetadata++;
            }
        }

    } catch (const std::exception& e) {
        logs::LogError("Failed to get statistics: " + std::string(e.what()));
    }

    return stats;
}

VideoInfo VideoLibrary::ScanVideoFile(const std::string& videoPath) {
    VideoInfo info;
    info.filePath = videoPath;
    info.filePathString = videoPath;
    info.name = fs::path(videoPath).filename().string();

    try {
        if (fs::exists(videoPath)) {
            info.fileSize = fs::file_size(videoPath);
        } else {
            logs::LogWarning("File does not exist: " + videoPath);
            return info;
        }

        // TODO: Move FFmpeg scanning logic here
        // For now, return basic info with file size

    } catch (const std::exception& e) {
        logs::LogWarning("Failed to scan video file: " + std::string(e.what()));
    }

    return info;
}

std::optional<std::string> VideoLibrary::GenerateThumbnail(const std::string& videoPath) const {
    if (!m_thumbnailService) {
        logs::LogWarning("ThumbnailService not available");
        return std::nullopt;
    }

    try {
        std::string thumbPath = m_thumbnailService->GenerateThumbnail(
            videoPath,
            ThumbnailStrategy::FRAME_AT_1SEC,
            320,
            180
        );

        if (!thumbPath.empty() && fs::exists(thumbPath)) {
            return thumbPath;
        }

        return std::nullopt;

    } catch (const std::exception& e) {
        logs::LogError("Failed to generate thumbnail: " + std::string(e.what()));
        return std::nullopt;
    }
}

void VideoLibrary::EnsureDirectoriesExist() const {
    try {
        if (!m_paths.thumbFolder.empty() && !fs::exists(m_paths.thumbFolder)) {
            logs::LogInfo("Creating thumbnail folder: " + m_paths.thumbFolder.string());
            fs::create_directories(m_paths.thumbFolder);
        }

        if (const auto dbDir = m_paths.dbPath.parent_path();
            !dbDir.empty() && !fs::exists(dbDir)) {
            logs::LogInfo("Creating database folder: " + dbDir.string());
            fs::create_directories(dbDir);
        }

    } catch (const std::exception& e) {
        logs::LogError("Could not create directories: " + std::string(e.what()));
    }
}