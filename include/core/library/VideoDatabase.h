#pragma once

#include "data/VideoInfo.h"
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

struct sqlite3;

class VideoDatabase {
public:
    explicit VideoDatabase(const std::string& dbPath);
    ~VideoDatabase();

    // Main function
    VideoInfo *GetMetadata(const std::string &filePath);
    void QueueForScanning(const std::string& filePath);
    void SaveMetadata(const VideoInfo& videoInfo);
    std::vector<VideoInfo> GetAllVideos();


    // Search and Filters
    std::vector<VideoInfo> SearchByName(const std::string& query);
    std::vector<VideoInfo> GetVideosByTag(const std::string& tag);

    bool IsScanning() const;
    bool VideoExists(const std::string& filePath);

    void ClearCache();
    void DeleteMetadata(const std::string& filePath);

    size_t GetQueueSize() const;

private:
    // Database
    void InitializeDatabase() const;
    void LoadCacheFromDB();

    bool LoadFromDB(const std::string& filePath, VideoInfo& info) const;
    void SaveToDB(const VideoInfo& info) const;

    // Background
    void BackgroundWorker();

    static VideoInfo ExtractVideoMetadata(const std::string& filePath);
    static std::string CalculateFileHash();

    sqlite3* db;
    std::unordered_map<std::string, VideoInfo> memoryCache;
    std::mutex cacheMutex;
    std::queue<std::string> scanQueue;
    std::thread workerThread;
    bool running;
};