#pragma once

#include "data/VideoInfo.h"

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>

class VideoLibrary;

enum class ImportStatus {
    SCANNING,
    GENERATING_THUMBNAIL,
    EMBEDDING_METADATA,
    SAVING_TO_DB,
    COMPLETED,
    FAILED
};

struct ImportProgress {
    std::string videoPath;
    ImportStatus status;
    float progress;
    std::string message;
};

using ProgressCallback = std::function<void(const ImportProgress&)>;

class VideoImportService {
public:
    VideoImportService();
    ~VideoImportService();

    VideoImportService(const VideoImportService&) = delete;
    VideoImportService& operator=(const VideoImportService&) = delete;

    void ImportVideo(
        const std::string& videoPath,
        VideoLibrary* library,
        const ProgressCallback& callback = nullptr
    );

    void ImportFolder(
        const std::string& folderPath,
        VideoLibrary* library,
        const ProgressCallback& callback = nullptr
    );

    void ImportMultiple(
        const std::vector<std::string>& videoPaths,
        VideoLibrary* library,
        const ProgressCallback& callback = nullptr
    );

    bool IsImporting() const;
    size_t GetQueueSize() const;
    void CancelImport();
    void WaitForCompletion() const;


    static std::vector<std::string> FindVideoFiles(const std::string& folderPath);

private:
    struct ImportTask {
        std::string videoPath;
        VideoLibrary* library;
        ProgressCallback callback;
    };

    void ImportWorkerThread();
    void ProcessImportTask(const ImportTask& task);
    VideoInfo ScanVideoWithFFmpeg(const std::string& videoPath);

    std::queue<ImportTask> m_importQueue;
    std::thread m_importWorker;
    mutable std::mutex m_queueMutex;
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_cancelRequested;
};