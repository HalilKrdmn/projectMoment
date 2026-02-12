#include "core/library/LibraryLoader.h"
#include "core/library/VideoLibrary.h"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

// Progress Constants
namespace lL{
    constexpr float SCAN_PROGRESS = 0.1f;
    constexpr float LOAD_START = 0.1f;
    constexpr float LOAD_END = 0.8f;
    constexpr float THUMBNAIL_PROGRESS = 0.95f;
    constexpr float COMPLETE_PROGRESS = 1.0f;

    // Logging Helper
    void NotifyProgress(const LibraryLoader::ProgressCallback& callback,
                       const std::string& message,
                       const float progress) {
        if (callback) {
            callback(message, progress);
        }
    }

    std::vector<fs::path> ScanVideoFiles(const std::string& libraryPath,
                                         const LibraryLoader::ProgressCallback& onProgress) {
        std::vector<fs::path> videoFiles;

        try {
            NotifyProgress(onProgress, "Scanning folder...", 0.0f);

            for (const auto& entry : fs::directory_iterator(libraryPath)) {

                if (entry.is_regular_file()) {
                    if (VideoLibrary::IsVideoFile(entry.path())) {
                        videoFiles.push_back(entry.path());
                    }
                }
            }

        } catch (const fs::filesystem_error& e) {
            NotifyProgress(onProgress,
                          std::string("Error scanning folder: ") + e.what(),
                          -1.0f);
        }

        return videoFiles;
    }

    void LoadVideos(VideoLibrary* library,
                   const std::vector<fs::path>& videoFiles,
                   const LibraryLoader::ProgressCallback& onProgress) {
        if (videoFiles.empty()) {
            NotifyProgress(onProgress, "No videos found.", COMPLETE_PROGRESS);
            return;
        }

        NotifyProgress(onProgress,
                      std::to_string(videoFiles.size()) + " video(s) found.",
                      SCAN_PROGRESS);

        for (size_t i = 0; i < videoFiles.size(); ++i) {
            const auto& videoFile = videoFiles[i];
            const std::string fileName = videoFile.filename().string();

            const float progress = LOAD_START +
                                 (LOAD_END - LOAD_START) *
                                 (static_cast<float>(i) / videoFiles.size());

            NotifyProgress(onProgress, "Loading: " + fileName, progress);

            try {
                library->LoadVideo(videoFile.string());
            } catch (const std::exception& e) {
                NotifyProgress(onProgress,
                              std::string("Error loading: ") + fileName +
                              " (" + e.what() + ")",
                              progress);
            }
        }
    }

    void GenerateThumbnails(VideoLibrary* library,
                           const LibraryLoader::ProgressCallback& onProgress) {
        try {
            NotifyProgress(onProgress, "Generating thumbnails...", THUMBNAIL_PROGRESS);
            library->RegenerateMissingThumbnails();
        } catch (const std::exception& e) {
            NotifyProgress(onProgress,
                          std::string("Error generating thumbnails: ") + e.what(),
                          -1.0f);
        }
    }
}

// Public API
void LibraryLoader::Run(VideoLibrary* library,
                        const std::string& libraryPath,
                        const ProgressCallback& onProgress) {
    // Validate input
    if (!library || libraryPath.empty()) {
        lL::NotifyProgress(onProgress, "Invalid library or path.", -1.0f);
        return;
    }

    // Validate path exists and is a directory
    if (!fs::exists(libraryPath)) {
        lL::NotifyProgress(onProgress, "Library path does not exist.", -1.0f);
        return;
    }

    if (!fs::is_directory(libraryPath)) {
        lL::NotifyProgress(onProgress, "Library path is not a directory.", -1.0f);
        return;
    }

    const auto videoFiles = lL::ScanVideoFiles(libraryPath, onProgress);

    if (videoFiles.empty()) {
        lL::NotifyProgress(onProgress, "No videos found in selected folder.", lL::COMPLETE_PROGRESS);
        return;
    }

    // Load videos into library
    lL::LoadVideos(library, videoFiles, onProgress);

    // Generate missing thumbnails
    lL::GenerateThumbnails(library, onProgress);

    // Complete
    lL::NotifyProgress(onProgress, "Library ready!", lL::COMPLETE_PROGRESS);
}