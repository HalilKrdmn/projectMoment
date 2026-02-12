#include "core/library/VideoDatabase.h"

#include <sqlite3.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <ranges>

namespace fs = std::filesystem;

VideoDatabase::VideoDatabase(const std::string &dbPath) : db(nullptr), running(true) {
    std::cout << "Opening database: " << dbPath << std::endl;
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

    int result = sqlite3_open(dbPath.c_str(), &db);

    if (result != SQLITE_OK) {
        std::cerr << "ERROR: Failed to open database!" << std::endl;
        std::cerr << "SQLite error code: " << result << std::endl;
        std::cerr << "SQLite error message: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("Database open failed");
    }

    std::cout << "Database opened successfully at: " << dbPath << std::endl;
    std::cout << "Database file exists: " << (std::filesystem::exists(dbPath) ? "YES" : "NO") << std::endl;

    InitializeDatabase();
    LoadCacheFromDB();
}

VideoDatabase::~VideoDatabase() {
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
    sqlite3_close(db);
}

void VideoDatabase::InitializeDatabase() const {
    const auto createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS videos (
            file_path TEXT PRIMARY KEY,
            file_name TEXT,
            file_size INTEGER,
            last_modified INTEGER,
            file_hash TEXT,

            duration_sec REAL,
            frame_rate INTEGER,
            resolution_width INTEGER,
            resolution_height INTEGER,
            file_resolution_width INTEGER,
            file_resolution_height INTEGER,

            thumbnail_path TEXT,

            description TEXT,
            tags TEXT,
            clip_start_point REAL,
            clip_end_point REAL,

            recording_time_ms INTEGER,
            last_edit_time_ms INTEGER,

            audio_codec TEXT,
            audio_bitrate INTEGER,
            audio_sample_rate INTEGER,
            audio_track_names TEXT,

            app_version TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_file_name ON videos(file_name);
        CREATE INDEX IF NOT EXISTS idx_last_modified ON videos(last_modified);
        CREATE INDEX IF NOT EXISTS idx_tags ON videos(tags);
        PRAGMA journal_mode=WAL;
        PRAGMA synchronous=NORMAL;
        PRAGMA cache_size=10000;
    )";

    char* errMsg = nullptr;
    sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    if (errMsg) {
        sqlite3_free(errMsg);
    }
}

VideoInfo* VideoDatabase::GetMetadata(const std::string& filePath) {
    std::lock_guard lock(cacheMutex);

    const auto it = memoryCache.find(filePath);
    if (it != memoryCache.end()) {
        return &it->second;
    }

    if (VideoInfo info; LoadFromDB(filePath, info)) {
        memoryCache[filePath] = info;
        return &memoryCache[filePath];
    }

    return nullptr;
}

void VideoDatabase::QueueForScanning(const std::string& filePath) {
    std::lock_guard lock(cacheMutex);
    scanQueue.push(filePath);
}

void VideoDatabase::SaveMetadata(const VideoInfo& videoInfo) {
    std::cout << "SaveMetadata START: " << videoInfo.filePathString << std::endl;

    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        std::cout << "  Mutex locked" << std::endl;

        memoryCache[videoInfo.filePathString] = videoInfo;
        std::cout << "  Added to memory cache (size: " << memoryCache.size() << ")" << std::endl;
    }
    std::cout << "  Mutex unlocked" << std::endl;

    std::cout << "  Calling SaveToDB..." << std::endl;
    SaveToDB(videoInfo);
    std::cout << "  SaveToDB completed" << std::endl;

    std::cout << "SaveMetadata END" << std::endl;
}

std::vector<VideoInfo> VideoDatabase::GetAllVideos() {
    std::lock_guard lock(cacheMutex);

    std::vector<VideoInfo> result;
    result.reserve(memoryCache.size());

    for (const auto &val: memoryCache | std::views::values) {
        result.push_back(val);
    }
    return result;
}

std::vector<VideoInfo> VideoDatabase::SearchByName(const std::string& query) {
    std::lock_guard lock(cacheMutex);

    std::vector<VideoInfo> results;
    for (const auto &val: memoryCache | std::views::values) {
        if (val.name.find(query) != std::string::npos) {
            results.push_back(val);
        }
    }
    return results;
}

std::vector<VideoInfo> VideoDatabase::GetVideosByTag(const std::string& tag) {
    std::lock_guard lock(cacheMutex);

    std::vector<VideoInfo> results;
    for (const auto &val: memoryCache | std::views::values) {
        if (val.tagsStorage.find(tag) != std::string::npos) {
            results.push_back(val);
        }
    }
    return results;
}

bool VideoDatabase::IsScanning() const {
    return !scanQueue.empty();
}

size_t VideoDatabase::GetQueueSize() const {
    return scanQueue.size();
}

void VideoDatabase::ClearCache() {
    std::lock_guard lock(cacheMutex);
    memoryCache.clear();
}

bool VideoDatabase::VideoExists(const std::string& filePath) {
    std::lock_guard lock(cacheMutex);
    return memoryCache.contains(filePath);
}

void VideoDatabase::LoadCacheFromDB() {
    sqlite3_stmt* stmt;

    if (const auto selectSQL = "SELECT * FROM videos;"; sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            VideoInfo info;

            info.filePathString = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            info.filePath = info.filePathString;
            info.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

            info.durationSec = sqlite3_column_double(stmt, 5);
            info.frameRate = sqlite3_column_int(stmt, 6);
            info.resolutionWidth = sqlite3_column_int(stmt, 7);
            info.resolutionHeight = sqlite3_column_int(stmt, 8);
            info.fileResolutionWidth = sqlite3_column_int(stmt, 9);
            info.fileResolutionHeight = sqlite3_column_int(stmt, 10);

            info.thumbnailPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
            info.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            info.tagsStorage = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));

            info.clipStartPoint = sqlite3_column_double(stmt, 14);
            info.clipEndPoint = sqlite3_column_double(stmt, 15);
            info.recordingTimeMs = sqlite3_column_int64(stmt, 16);
            info.lastEditTimeMs = sqlite3_column_int64(stmt, 17);

            info.audioCodec = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18));
            info.audioBitrate = sqlite3_column_int(stmt, 19);
            info.audioSampleRate = sqlite3_column_int(stmt, 20);
            info.audioTrackNamesStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21));

            info.appVersion = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 22));

            memoryCache[info.filePathString] = info;
        }
        sqlite3_finalize(stmt);
    }
}

bool VideoDatabase::LoadFromDB(const std::string& filePath, VideoInfo& info) const {
    sqlite3_stmt* stmt;
    bool found = false;

    if (const auto selectSQL = "SELECT * FROM videos WHERE file_path = ?;"; sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            info.filePathString = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            info.filePath = info.filePathString;
            info.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

            info.durationSec = sqlite3_column_double(stmt, 5);
            info.frameRate = sqlite3_column_int(stmt, 6);
            info.resolutionWidth = sqlite3_column_int(stmt, 7);
            info.resolutionHeight = sqlite3_column_int(stmt, 8);
            info.fileResolutionWidth = sqlite3_column_int(stmt, 9);
            info.fileResolutionHeight = sqlite3_column_int(stmt, 10);

            info.thumbnailPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
            info.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            info.tagsStorage = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));

            info.clipStartPoint = sqlite3_column_double(stmt, 14);
            info.clipEndPoint = sqlite3_column_double(stmt, 15);
            info.recordingTimeMs = sqlite3_column_int64(stmt, 16);
            info.lastEditTimeMs = sqlite3_column_int64(stmt, 17);

            info.audioCodec = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 18));
            info.audioBitrate = sqlite3_column_int(stmt, 19);
            info.audioSampleRate = sqlite3_column_int(stmt, 20);
            info.audioTrackNamesStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 21));

            info.appVersion = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 22));

            found = true;
        }
        sqlite3_finalize(stmt);
    }

    return found;
}

void VideoDatabase::SaveToDB(const VideoInfo& info) const {
    std::cout << "SaveToDB START: " << info.filePathString << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    const auto insertSQL = R"(
        INSERT OR REPLACE INTO videos VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
    )";

    sqlite3_stmt* stmt;

    std::cout << "Preparing SQL statement..." << std::endl;
    int prepareResult = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
    std::cout << "Prepare result: " << prepareResult << " (SQLITE_OK=" << SQLITE_OK << ")" << std::endl;

    if (prepareResult == SQLITE_OK) {
        std::cout << "Binding parameters..." << std::endl;

        sqlite3_bind_text(stmt, 1, info.filePathString.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, info.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, 0);
        sqlite3_bind_int64(stmt, 4, 0);
        sqlite3_bind_text(stmt, 5, "", -1, SQLITE_STATIC);

        sqlite3_bind_double(stmt, 6, info.durationSec);
        sqlite3_bind_int(stmt, 7, info.frameRate);
        sqlite3_bind_int(stmt, 8, info.resolutionWidth);
        sqlite3_bind_int(stmt, 9, info.resolutionHeight);
        sqlite3_bind_int(stmt, 10, info.fileResolutionWidth);
        sqlite3_bind_int(stmt, 11, info.fileResolutionHeight);

        sqlite3_bind_text(stmt, 12, info.thumbnailPath.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 13, info.description.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 14, info.tagsStorage.c_str(), -1, SQLITE_STATIC);

        sqlite3_bind_double(stmt, 15, info.clipStartPoint);
        sqlite3_bind_double(stmt, 16, info.clipEndPoint);
        sqlite3_bind_int64(stmt, 17, info.recordingTimeMs);
        sqlite3_bind_int64(stmt, 18, info.lastEditTimeMs);

        sqlite3_bind_text(stmt, 19, info.audioCodec.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 20, info.audioBitrate);
        sqlite3_bind_int(stmt, 21, info.audioSampleRate);
        sqlite3_bind_text(stmt, 22, info.audioTrackNamesStr.c_str(), -1, SQLITE_STATIC);

        sqlite3_bind_text(stmt, 23, info.appVersion.c_str(), -1, SQLITE_STATIC);

        std::cout << "Parameters bound, executing..." << std::endl;
        int stepResult = sqlite3_step(stmt);
        std::cout << "Step result: " << stepResult << " (SQLITE_DONE=" << SQLITE_DONE << ")" << std::endl;

        std::cout << "Finalizing statement..." << std::endl;
        sqlite3_finalize(stmt);
        std::cout << "Statement finalized" << std::endl;
    } else {
        std::cout << "SQL prepare FAILED: " << sqlite3_errmsg(db) << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "SaveToDB END - Duration: " << duration.count() << "ms" << std::endl;
}




void VideoDatabase::BackgroundWorker() {
    std::cout << "BackgroundWorker DISABLED (causing deadlock)" << std::endl;

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "BackgroundWorker ended" << std::endl;

}


VideoInfo VideoDatabase::ExtractVideoMetadata(const std::string& filePath) {
    VideoInfo info;
    info.filePath = filePath;
    info.filePathString = filePath;

    const fs::path p(filePath);
    info.name = p.filename().string();

    // TODO: FFmpeg extract video metadata
    // avformat_open_input, avformat_find_stream_info

    return info;
}

std::string VideoDatabase::CalculateFileHash() {
    // TODO: SHA256 hash
    return "";
}