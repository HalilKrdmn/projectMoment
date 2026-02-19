#include "core/import/VideoImportService.h"

#include "core/library/VideoLibrary.h"
#include "core/library/VideoDatabase.h"
#include "core/media/ThumbnailService.h"
#include "core/media/MetadataEmbedder.h"

#include <filesystem>
#include <iostream>
#include <chrono>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}


namespace fs = std::filesystem;

namespace logs{
    inline void LogInfo(const std::string& msg) {
        std::cout << "[VideoImportService] " << msg << std::endl;
    }

    inline void LogError(const std::string& msg) {
        std::cerr << "[VideoImportService] ERROR: " << msg << std::endl;
    }

    void LogWorker(const std::string& msg) {
        std::cout << "[ImportWorker] " << msg << std::endl;
    }
}


VideoImportService::VideoImportService()
    : m_isRunning(true)
    , m_cancelRequested(false) {

    logs::LogInfo("Starting worker thread...");
    m_importWorker = std::thread(&VideoImportService::ImportWorkerThread, this);
    logs::LogInfo("Ready!");
}

VideoImportService::~VideoImportService() {
    logs::LogInfo("Shutting down...");

    m_isRunning = false;

    if (m_importWorker.joinable()) {
        m_importWorker.join();
    }

    logs::LogInfo("Destroyed");
}


void VideoImportService::ImportVideo(
    const std::string& videoPath,
    VideoLibrary* library,
    const ProgressCallback& callback) {
    
    if (!library) {
        logs::LogError("VideoLibrary is null");
        return;
    }

    logs::LogInfo("Queueing: " + videoPath);

    ImportTask task;
    task.videoPath = videoPath;
    task.library = library;
    task.callback = callback;

    {
        std::lock_guard lock(m_queueMutex);
        m_importQueue.push(task);
        logs::LogInfo("Queue size: " + std::to_string(m_importQueue.size()));
    }
}

void VideoImportService::ImportFolder(
    const std::string& folderPath,
    VideoLibrary* library,
    const ProgressCallback& callback) {

    if (!library) {
        logs::LogError("VideoLibrary is null");
        return;
    }

    logs::LogInfo("Scanning folder: " + folderPath);

    const auto videoFiles = FindVideoFiles(folderPath);

    logs::LogInfo("Found " + std::to_string(videoFiles.size()) + " videos");

    // Queue each video
    for (const auto& videoPath : videoFiles) {
        ImportVideo(videoPath, library, callback);
    }

    logs::LogInfo("All videos queued");
}

void VideoImportService::ImportMultiple(
    const std::vector<std::string>& videoPaths,
    VideoLibrary* library,
    const ProgressCallback& callback) {
    
    if (!library) {
        logs::LogError("VideoLibrary is null");
        return;
    }

    logs::LogInfo("Queueing " + std::to_string(videoPaths.size()) + " videos");
    
    for (const auto& videoPath : videoPaths) {
        ImportVideo(videoPath, library, callback);
    }
}

// QUEUE CONTROL
bool VideoImportService::IsImporting() const {
    std::lock_guard lock(m_queueMutex);
    return !m_importQueue.empty();
}

size_t VideoImportService::GetQueueSize() const {
    std::lock_guard lock(m_queueMutex);
    return m_importQueue.size();
}

void VideoImportService::CancelImport() {
    logs::LogInfo("Cancel requested");
    m_cancelRequested = true;

    std::lock_guard lock(m_queueMutex);
    while (!m_importQueue.empty()) {
        m_importQueue.pop();
    }
}

void VideoImportService::WaitForCompletion() const {
    logs::LogInfo("Waiting for completion...");

    while (IsImporting()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    logs::LogInfo("All imports complete");
}


// WORKER THREAD
void VideoImportService::ImportWorkerThread() {
    logs::LogWorker("Started");

    while (m_isRunning) {
        ImportTask task;
        bool hasTask = false;

        // Get task from queue
        {
            std::lock_guard lock(m_queueMutex);
            if (!m_importQueue.empty()) {
                task = m_importQueue.front();
                m_importQueue.pop();
                hasTask = true;
                logs::LogWorker("Processing: " + task.videoPath);
                logs::LogWorker("Remaining: " + std::to_string(m_importQueue.size()));
            }
        }

        // No task, sleep
        if (!hasTask) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Check cancel
        if (m_cancelRequested) {
            logs::LogWorker("Cancelled, skipping task");
            m_cancelRequested = false;
            continue;
        }

        // Process task (outside mutex)
        ProcessImportTask(task);
    }

    logs::LogWorker("Stopped");
}

void VideoImportService::ProcessImportTask(const ImportTask& task) {
    std::cout << "[ProcessTask] START: " << task.videoPath << std::endl;

    ImportProgress progress;
    progress.videoPath = task.videoPath;
    progress.progress = 0.0f;

    try {
        // STEP 1: SCANNING
        std::cout << "  [1/5] Scanning with FFmpeg..." << std::endl;
        progress.status = ImportStatus::SCANNING;
        progress.message = "Scanning video...";
        progress.progress = 0.1f;
        if (task.callback) task.callback(progress);

        VideoInfo info = ScanVideoWithFFmpeg(task.videoPath);
        std::cout << "  Video: " << info.resolutionWidth << "x" << info.resolutionHeight
                  << ", " << info.durationSec << "s" << std::endl;

        // STEP 2: THUMBNAIL
        std::cout << "  [2/5] Generating thumbnail..." << std::endl;
        progress.status = ImportStatus::GENERATING_THUMBNAIL;
        progress.message = "Creating thumbnail...";
        progress.progress = 0.4f;
        if (task.callback) task.callback(progress);

        auto thumbService = task.library->GetThumbnailService();
        if (thumbService) {
            info.thumbnailPath = thumbService->GenerateThumbnail(task.videoPath);
            std::cout << "  Thumbnail: " << info.thumbnailPath << std::endl;
        }

        // STEP 3: EMBED METADATA
        std::cout << "  [3/5] Embedding metadata..." << std::endl;
        progress.status = ImportStatus::EMBEDDING_METADATA;
        progress.message = "Writing metadata to video...";
        progress.progress = 0.6f;
        if (task.callback) task.callback(progress);

        auto embedder = task.library->GetMetadataEmbedder();
        if (embedder) {
            if (!embedder->HasEmbeddedMetadata(task.videoPath)) {
                embedder->WriteMetadataToVideo(task.videoPath, info);
                std::cout << "  Metadata embedded" << std::endl;
            } else {
                std::cout << "  Metadata already exists" << std::endl;
            }
        }

        // STEP 4: SAVE TO DATABASE
        std::cout << "  [4/5] Saving to database..." << std::endl;
        progress.status = ImportStatus::SAVING_TO_DB;
        progress.message = "Saving to database...";
        progress.progress = 0.9f;
        if (task.callback) task.callback(progress);

        auto database = task.library->GetDatabase();
        if (database) {
            database->SaveMetadata(info);
            std::cout << "  Saved to database" << std::endl;
        }

        // STEP 5: COMPLETED
        std::cout << "  [5/5] Completed!" << std::endl;
        progress.status = ImportStatus::COMPLETED;
        progress.message = "Import complete!";
        progress.progress = 1.0f;
        if (task.callback) task.callback(progress);

        std::cout << "[ProcessTask] SUCCESS" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[ProcessTask] ERROR: " << e.what() << std::endl;

        progress.status = ImportStatus::FAILED;
        progress.message = std::string("Error: ") + e.what();
        progress.progress = 0.0f;
        if (task.callback) task.callback(progress);
    }
}

VideoInfo VideoImportService::ScanVideoWithFFmpeg(const std::string& videoPath) {
    VideoInfo info;
    info.filePath = videoPath;
    info.filePathString = videoPath;
    info.name = fs::path(videoPath).filename().string();

    // Get file size and modification time
    try {
        if (fs::exists(videoPath)) {
            info.fileSize = fs::file_size(videoPath);
            auto ftime = fs::last_write_time(videoPath);
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            info.lastModified = std::chrono::system_clock::to_time_t(sctp);
        }
    } catch (const std::exception& e) {
        logs::LogError("File stat error: " + std::string(e.what()));
        info.fileSize = 0;
        info.lastModified = 0;
    }

    // Open video with FFmpeg
    AVFormatContext* formatCtx = nullptr;

    try {
        int ret = avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr);
        if (ret < 0) {
            char errbuf[256];
            av_strerror(ret, errbuf, sizeof(errbuf));
            throw std::runtime_error(std::string("Failed to open video: ") + errbuf);
        }

        ret = avformat_find_stream_info(formatCtx, nullptr);
        if (ret < 0) {
            avformat_close_input(&formatCtx);
            char errbuf[256];
            av_strerror(ret, errbuf, sizeof(errbuf));
            throw std::runtime_error(std::string("Failed to find stream info: ") + errbuf);
        }

        // Duration
        if (formatCtx->duration != AV_NOPTS_VALUE) {
            info.durationSec = static_cast<double>(formatCtx->duration) / AV_TIME_BASE;
        }

        // Find video stream
        bool foundVideo = false;
        for (unsigned i = 0; i < formatCtx->nb_streams; i++) {
            const AVStream* stream = formatCtx->streams[i];
            if (!stream || !stream->codecpar) continue;

            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                info.resolutionWidth  = stream->codecpar->width;
                info.resolutionHeight = stream->codecpar->height;
                foundVideo = true;
                break;
            }
        }

        avformat_close_input(&formatCtx);

        if (!foundVideo) {
            throw std::runtime_error("No video stream found in file");
        }

    } catch (const std::exception& e) {
        if (formatCtx) {
            avformat_close_input(&formatCtx);
        }
        throw;
    }

    return info;
}

std::vector<std::string> VideoImportService::FindVideoFiles(const std::string& folderPath) {
    std::vector<std::string> videoFiles;

    try {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file() &&
                VideoLibrary::IsVideoFile(entry.path())) {
                videoFiles.push_back(entry.path().string());
                }
        }
    } catch (const std::exception& e) {
        logs::LogError("FindVideoFiles error: " + std::string(e.what()));
    }

    return videoFiles;
}
