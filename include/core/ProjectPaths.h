#pragma once

#include <filesystem>
#include <iostream>

struct ProjectPaths {
    std::filesystem::path rootFolder;
    std::filesystem::path momentFolder;

    std::filesystem::path dbPath;
    std::filesystem::path thumbFolder;

    static ProjectPaths FromFolder(const std::filesystem::path& folder) {
        ProjectPaths p;
        p.rootFolder = folder;
        p.momentFolder = folder / ".moment";
        p.dbPath = p.momentFolder / "library.db";
        p.thumbFolder = p.momentFolder / "thumbnails";
        return p;
    }

    bool CreateFolders() const {
        try {
            std::filesystem::create_directories(momentFolder);
            std::filesystem::create_directories(thumbFolder);
            return true;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[ProjectPaths] Failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool DatabaseExists() const {
        return std::filesystem::exists(dbPath);
    }

    static std::string GetThumbFilename(const std::filesystem::path& videoPath) {
        return videoPath.stem().string() + ".jpg";
    }

    std::filesystem::path GetThumbPath(const std::string& filename) const {
        return thumbFolder / filename;
    }
};