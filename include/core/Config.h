#pragma once

#include "core/media/AudioDeviceEnumerator.h"

#include <iostream>
#include <string>
#include <filesystem>
#include <optional>

#include <toml++/toml.hpp>

// ──────────────────────────────────────────────────────────────────────────
struct AudioTrack {
    std::string     name;
    std::string     device;
    AudioDeviceType deviceType = AudioDeviceType::Input;
};

enum class AudioMode { Mixed, Separated, Virtual };

namespace fs = std::filesystem;
// ──────────────────────────────────────────────────────────────────────────

struct Config {

    // ─── GENERAL SETTINGS ───────────────────────────────────────────────────────

    std::string appVersion                      = "0.0.1-11022026";
    bool startMinimized                         = false;
    std::string libraryPath;

    // ─── RECORDING SETTINGS ─────────────────────────────────────────────────────

    std::string recordingMode                   = "native";  // "obs" or "native"
    bool recordingAutoStart                     = false;
    std::string hotkeyRecordToggle              = "F10";
    std::string hotkeySaveClip                  = "F11";
    std::string hotkeyToggleMic                 = "F12";

    // OBS
    std::string obsHost                         = "localhost";
    int obsPort                                 = 4455;

    // Native Recording
    AudioMode nativeAudioMode                   = AudioMode::Mixed; // "mixed", "separated" and "virtual"
    std::vector<AudioTrack> nativeAudioTracks   = {};
    std::string nativeScreenOutput              = "";
    std::string nativeVideoCodec                = "libx264";
    std::string nativeAudioCodec                = "aac";
    int nativeVideoBitrate                      = 5000;
    int nativeAudioBitrate                      = 192;
    int nativeFPS                               = 30;
    int clipDuration                            = 30;

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
