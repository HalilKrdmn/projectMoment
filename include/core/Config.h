#pragma once

#include <toml++/toml.hpp>

#include <iostream>
#include <string>
#include <filesystem>
#include <optional>


struct AudioTrack {
    std::string name;
    std::string device;
};

enum class AudioMode { Mixed, Separated, Virtual };

namespace fs = std::filesystem;


struct Config {

    /*
     *  GENERAL SETTINGS
     */

    // App info
    std::string appVersion = "0.0.1-11022026";
    std::string appName = "ProjectMoment";

    // Library
    std::string libraryPath;


    /*
     *  RECORDING SETTINGS
     */

    // General
    std::string recordingMode = "native";  // "obs" or "native"
    std::string recordingHotkey = "F9";

    // OBS
    std::string obsHost = "localhost";
    int obsPort = 4455;
    std::string obsPassword = "";
    bool obsAutoStart = true;
    bool obsAskBeforeClosing = true;
    bool obsRememberChoice = false;
    int obsReplayBufferDuration = 60;

    // Native Recording
    AudioMode nativeAudioMode = AudioMode::Mixed;
    std::vector<AudioTrack> nativeAudioTracks = {{"Desktop", "default"}};
    std::string nativeScreenOutput = "";
    std::string nativeVideoCodec = "libx264";
    std::string nativeAudioCodec = "aac";
    int nativeVideoBitrate = 5000;
    int nativeAudioBitrate = 192;
    int nativeFPS = 30;

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
