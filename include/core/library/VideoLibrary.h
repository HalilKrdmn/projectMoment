#pragma once

#include "core/VideoInfo.h"

#include "core/CoreServices.h"

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


// Forward declarations
class VideoDatabase;
class ThumbnailService;
class MetadataEmbedder;
class VideoScanner;

class VideoLibrary {
public:
    explicit VideoLibrary(const ProjectPaths& paths);
    ~VideoLibrary();

    VideoLibrary(const VideoLibrary&) = delete;
    VideoLibrary& operator=(const VideoLibrary&) = delete;

    static constexpr std::array<std::string_view, 7> SUPPORTED_VIDEO_FORMATS{
        ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm"
    };
    static bool IsVideoFile(const fs::path& filePath);


    // Query Operations
    std::vector<VideoInfo> GetAllVideos() const;
    std::optional<std::reference_wrapper<const VideoInfo>> GetVideo(const std::string& filePath) const;
    std::vector<VideoInfo> SearchByName(const std::string& query) const;
    std::vector<VideoInfo> FilterByDuration(double minSec, double maxSec) const;
    std::vector<VideoInfo> FilterByResolution(int minWidth, int minHeight) const;


    VideoInfo LoadVideo(const std::string& videoPath);
    bool SaveVideo(const VideoInfo& info);
    bool UpdateVideo(const VideoInfo& info, bool updateVideoFile = false);
    bool DeleteVideo(const std::string& filePath, bool deleteFromDisk = false);

    // Maintenance Operations
    void RegenerateMissingThumbnails();
    void SyncWithVideoFiles() const;
    void CleanupOrphanedRecords();
    static bool IsVideoCorrupted(const std::string &videoPath);

    // Statistics
    struct Statistics {
        size_t totalVideos = 0;
        double totalDurationSec = 0.0;
        size_t totalSizeBytes = 0;
        size_t videosWithThumbnails = 0;
        size_t videosWithEmbeddedMetadata = 0;
    };

    Statistics GetStatistics() const;

    // Service Access
    VideoDatabase* GetDatabase() const { return m_database.get(); }
    ThumbnailService* GetThumbnailService() const { return m_thumbnailService.get(); }
    MetadataEmbedder* GetMetadataEmbedder() const { return m_metadataEmbedder.get(); }

private:
    // Helper methods
    static VideoInfo ScanVideoFile(const std::string& videoPath);

    std::optional<std::string> GenerateThumbnail(const std::string &videoPath) const;

    void EnsureDirectoriesExist() const;

    ProjectPaths m_paths;

    // Services
    std::unique_ptr<VideoDatabase> m_database;
    std::unique_ptr<ThumbnailService> m_thumbnailService;
    std::unique_ptr<MetadataEmbedder> m_metadataEmbedder;
};