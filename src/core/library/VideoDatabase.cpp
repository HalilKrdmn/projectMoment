#include "core/library/VideoDatabase.h"

#include <sqlite3.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <ranges>

namespace fs = std::filesystem;

VideoDatabase::VideoDatabase(const std::string &dbPath) : db(nullptr), running(true) {
    std::cout << "[VideoDatabase] Opening database: " << dbPath << std::endl;
    std::cout << "[VideoDatabase] Current working directory: " << std::filesystem::current_path() << std::endl;

    if (const int result = sqlite3_open(dbPath.c_str(), &db); result != SQLITE_OK) {
        std::cerr << "[VideoDatabase] ERROR: Failed to open database!" << std::endl;
        std::cerr << "[VideoDatabase] SQLite error code: " << result << std::endl;
        std::cerr << "[VideoDatabase] SQLite error message: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("[VideoDatabase] Database open failed");
    }

    std::cout << "[VideoDatabase] Database opened successfully at: " << dbPath << std::endl;
    std::cout << "[VideoDatabase] Database file exists: " << (std::filesystem::exists(dbPath) ? "YES" : "NO") << std::endl;

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
            file_path         TEXT PRIMARY KEY,
            file_name         TEXT,
            file_size         INTEGER,
            duration_sec      REAL,
            frame_rate        INTEGER,
            resolution_width  INTEGER,
            resolution_height INTEGER,
            thumbnail_path    TEXT,
            is_favorite       INTEGER,
            clip_start_point  REAL,
            clip_end_point    REAL,
            recording_time_ms INTEGER,
            last_edit_time_ms INTEGER,
            app_version       TEXT
        );
        CREATE INDEX IF NOT EXISTS idx_file_name ON videos(file_name);
        PRAGMA journal_mode=WAL;
        PRAGMA synchronous=NORMAL;
        PRAGMA cache_size=10000;
    )";

    char* errMsg = nullptr;
    sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    if (errMsg) {
        std::cerr << "[VideoDatabase] InitializeDatabase error: " << errMsg << std::endl;
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
    std::cout << "[VideoDatabase] " << videoInfo.filePathString << std::endl;

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

void VideoDatabase::DeleteMetadata(const std::string& filePath) {
    std::cout << "[VideoDatabase] " << filePath << std::endl;

    {
        std::lock_guard lock(cacheMutex);
        memoryCache.erase(filePath);
        std::cout << "  Removed from memory cache" << std::endl;
    }

    sqlite3_stmt* stmt;

    if (const auto deleteSQL = "DELETE FROM videos WHERE file_path = ?;"; sqlite3_prepare_v2(db, deleteSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            std::cout << "  Removed from database" << std::endl;
        } else {
            std::cout << "  Database deletion failed: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_finalize(stmt);
    }
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

bool VideoDatabase::IsScanning() const {
    return !scanQueue.empty();
}

bool VideoDatabase::VideoExists(const std::string& filePath) {
    std::lock_guard lock(cacheMutex);
    return memoryCache.contains(filePath);
}

size_t VideoDatabase::GetQueueSize() const {
    return scanQueue.size();
}

void VideoDatabase::ClearCache() {
    std::lock_guard lock(cacheMutex);
    memoryCache.clear();
}

void VideoDatabase::LoadCacheFromDB() {
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, "SELECT * FROM videos;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            VideoInfo info;

            info.filePathString   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            info.filePath         = info.filePathString;
            info.name             = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            info.fileSize         = sqlite3_column_int64(stmt, 2);

            info.durationSec      = sqlite3_column_double(stmt, 3);
            info.frameRate        = sqlite3_column_int(stmt, 4);
            info.resolutionWidth  = sqlite3_column_int(stmt, 5);
            info.resolutionHeight = sqlite3_column_int(stmt, 6);

            info.thumbnailPath    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            info.isFavorite       = sqlite3_column_int(stmt, 8) > 0;

            info.clipStartPoint   = sqlite3_column_double(stmt, 9);
            info.clipEndPoint     = sqlite3_column_double(stmt, 10);
            info.recordingTimeMs  = sqlite3_column_int64(stmt, 11);
            info.lastEditTimeMs   = sqlite3_column_int64(stmt, 12);

            info.appVersion       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));

            memoryCache[info.filePathString] = info;
        }
        sqlite3_finalize(stmt);
    }
}

bool VideoDatabase::LoadFromDB(const std::string& filePath, VideoInfo& info) const {
    sqlite3_stmt* stmt;
    bool found = false;

    if (sqlite3_prepare_v2(db, "SELECT * FROM videos WHERE file_path = ?;", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            info.filePathString   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            info.filePath         = info.filePathString;
            info.name             = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            info.fileSize         = sqlite3_column_int64(stmt, 2);

            info.durationSec      = sqlite3_column_double(stmt, 3);
            info.frameRate        = sqlite3_column_int(stmt, 4);
            info.resolutionWidth  = sqlite3_column_int(stmt, 5);
            info.resolutionHeight = sqlite3_column_int(stmt, 6);

            info.thumbnailPath    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            info.isFavorite       = sqlite3_column_int(stmt, 8) > 0;

            info.clipStartPoint   = sqlite3_column_double(stmt, 9);
            info.clipEndPoint     = sqlite3_column_double(stmt, 10);
            info.recordingTimeMs  = sqlite3_column_int64(stmt, 11);
            info.lastEditTimeMs   = sqlite3_column_int64(stmt, 12);

            info.appVersion       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));

            found = true;
        }
        sqlite3_finalize(stmt);
    }

    return found;
}

void VideoDatabase::SaveToDB(const VideoInfo& info) const {
    const auto insertSQL = R"(
        INSERT OR REPLACE INTO videos VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?);
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1,  info.filePathString.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2,  info.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, info.fileSize);

        sqlite3_bind_double(stmt, 4, info.durationSec);
        sqlite3_bind_int(stmt, 5,   info.frameRate);
        sqlite3_bind_int(stmt, 6,   info.resolutionWidth);
        sqlite3_bind_int(stmt, 7,   info.resolutionHeight);

        sqlite3_bind_text(stmt, 8,  info.thumbnailPath.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 9,   info.isFavorite ? 1 : 0);

        sqlite3_bind_double(stmt, 10, info.clipStartPoint);
        sqlite3_bind_double(stmt, 11, info.clipEndPoint);
        sqlite3_bind_int64(stmt, 12,  info.recordingTimeMs);
        sqlite3_bind_int64(stmt, 13,  info.lastEditTimeMs);

        sqlite3_bind_text(stmt, 14, info.appVersion.c_str(), -1, SQLITE_STATIC);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "[VideoDatabase] SaveToDB failed: " << sqlite3_errmsg(db) << std::endl;
    }
}

void VideoDatabase::BackgroundWorker() const {
    std::cout << "[VideoDatabase] BackgroundWorker DISABLED (causing deadlock)" << std::endl;

    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "[VideoDatabase] BackgroundWorker ended" << std::endl;

}

VideoInfo VideoDatabase::ExtractVideoMetadata(const std::string& filePath) {
    VideoInfo info;
    info.filePath = filePath;
    info.filePathString = filePath;

    const fs::path p(filePath);
    info.name = p.filename().string();

    // TODO: FFmpeg extract video metadata

    return info;
}