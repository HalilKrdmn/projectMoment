#pragma once

#include <toml++/toml.hpp>

#include <iostream>
#include <string>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

struct Config {
    // App info
    std::string appVersion = "0.0.1-11022026";
    std::string appName = "ProjectMoment";

    // Library
    std::string libraryPath;

    template<typename T>
    bool Set(const std::string &section, const std::string &key, const T &value) {
        return UpdateField(section, key, value);
    }

    static std::optional<Config> InitializeOrCreateConfig();

    bool Load();

    bool Save() const;

    static bool Exists();

    static fs::path GetSettingsPath();

private:
    template<typename T>
    static bool UpdateField(const std::string &section, const std::string &key, const T &value) {
        try {
            toml::table config;
            if (fs::exists(GetSettingsPath())) {
                config = toml::parse_file(GetSettingsPath().string());
            }

            toml::table* section_ptr = nullptr;
            if (auto existing = config[section].as_table()) {
                section_ptr = existing;
            } else {
                config.insert_or_assign(section, toml::table{});
                section_ptr = config[section].as_table();
            }

            if (section_ptr) {
                section_ptr->insert_or_assign(key, value);
            }

            fs::create_directories(GetSettingsPath().parent_path());
            std::ofstream file(GetSettingsPath());
            if (!file.is_open()) return false;

            file << config;
            std::cout << "[Config] Updated: " << section << "." << key << std::endl;
            return true;
        } catch (const std::exception &e) {
            std::cerr << "[Config] Update failed: " << e.what() << std::endl;
            return false;
        }
    }
};
