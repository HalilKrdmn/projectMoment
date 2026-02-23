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

enum class VideoCodec {
    H264, HEVC, AV1, VP8, VP9, HEVC_HDR, AV1_HDR, HEVC_10BIT, AV1_10BIT
};

enum class AudioCodec {
    AAC, OPUS, FLAC
};

enum class EncoderMode {
    GPU, CPU
};

enum class QualityPreset {
    Ultra, VeryHigh, High, Medium, Low
};

enum class BitrateMode {
    Auto, QP, VBR, CBR
};

enum class ReplayStorage {
    RAM, Disk
};

enum class ContainerFormat {
    MP4, MKV, FLV
};

enum class ColorRange {
    Limited, Full
};

enum class FramerateMode {
    CFR, VFR, Content
};

enum class TuneProfile {
    Performance, Quality
};

enum class AudioMode {
    Mixed, Separated, Virtual
};

namespace fs = std::filesystem;
// ──────────────────────────────────────────────────────────────────────────

struct Config {

    // ─── GENERAL SETTINGS ─────────────────────────────────────────────────

    std::string appVersion                      = "0.0.1-11022026";
    bool startMinimized                         = false;
    std::string libraryPath;

    // ─── RECORDING SETTINGS ───────────────────────────────────────────────
    std::string recordingMode                   = "native";     // "obs" or "native"
    bool recordingAutoStart                     = false;
    std::string hotkeyRecordToggle              = "F10";
    std::string hotkeySaveClip                  = "F11";
    std::string hotkeyToggleMic                 = "F12";

    // ─── OBS ──────────────────────────────────────────────────────────────
    std::string obsHost                         = "localhost";
    int obsPort                                 = 4455;

    // ─── NATIVE RECORDING ─────────────────────────────────────────────────
    std::string nativeScreenOutput              = "";

    // Codecs & Encoder
    VideoCodec nativeVideoCodec                 = VideoCodec::H264;
    AudioCodec nativeAudioCodec                 = AudioCodec::OPUS;
    EncoderMode nativeEncoder                   = EncoderMode::GPU;
    bool nativeFallbackCpu                      = true;

    // Quality & Bitrate
    QualityPreset nativeQuality                 = QualityPreset::VeryHigh;
    BitrateMode nativeBitrateMode               = BitrateMode::VBR;
    int nativeVideoBitrate                      = 5000;
    int nativeAudioBitrate                      = 192;
    int nativeFPS                               = 60;
    int nativeClipDuration                      = 60;

    // Advanced
    ReplayStorage nativeReplayStorage           = ReplayStorage::RAM;
    bool nativeShowCursor                       = true;
    ContainerFormat nativeContainerFormat       = ContainerFormat::MP4;
    ColorRange nativeColorRange                 = ColorRange::Limited;
    FramerateMode nativeFramerateMode           = FramerateMode::VFR;
    TuneProfile nativeTune                      = TuneProfile::Quality;

    // Audio
    AudioMode nativeAudioMode                   = AudioMode::Mixed;
    std::vector<AudioTrack> nativeAudioTracks   = {};

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
