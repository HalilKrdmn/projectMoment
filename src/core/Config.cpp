#include "core/Config.h"

#include <fstream>

// ─── Enum ↔ String helpers ────────────────────────────────────────────────────

static std::string VideoCodecToStr(VideoCodec v) {
    switch (v) {
        case VideoCodec::HEVC:      return "hevc";
        case VideoCodec::AV1:       return "av1";
        case VideoCodec::VP8:       return "vp8";
        case VideoCodec::VP9:       return "vp9";
        case VideoCodec::HEVC_HDR:  return "hevc_hdr";
        case VideoCodec::AV1_HDR:   return "av1_hdr";
        case VideoCodec::HEVC_10BIT:return "hevc_10bit";
        case VideoCodec::AV1_10BIT: return "av1_10bit";
        default:                    return "h264";
    }
}
static VideoCodec VideoCodecFromStr(const std::string& s) {
    if (s == "hevc")      return VideoCodec::HEVC;
    if (s == "av1")       return VideoCodec::AV1;
    if (s == "vp8")       return VideoCodec::VP8;
    if (s == "vp9")       return VideoCodec::VP9;
    if (s == "hevc_hdr")  return VideoCodec::HEVC_HDR;
    if (s == "av1_hdr")   return VideoCodec::AV1_HDR;
    if (s == "hevc_10bit")return VideoCodec::HEVC_10BIT;
    if (s == "av1_10bit") return VideoCodec::AV1_10BIT;
    return VideoCodec::H264;
}

static std::string AudioCodecToStr(AudioCodec v) {
    switch (v) {
        case AudioCodec::OPUS: return "opus";
        case AudioCodec::FLAC: return "flac";
        default:               return "aac";
    }
}
static AudioCodec AudioCodecFromStr(const std::string& s) {
    if (s == "opus") return AudioCodec::OPUS;
    if (s == "flac") return AudioCodec::FLAC;
    return AudioCodec::AAC;
}

static std::string EncoderModeToStr(EncoderMode v) {
    return (v == EncoderMode::CPU) ? "cpu" : "gpu";
}
static EncoderMode EncoderModeFromStr(const std::string& s) {
    return (s == "cpu") ? EncoderMode::CPU : EncoderMode::GPU;
}

static std::string QualityPresetToStr(QualityPreset v) {
    switch (v) {
        case QualityPreset::Ultra:    return "ultra";
        case QualityPreset::High:     return "high";
        case QualityPreset::Medium:   return "medium";
        case QualityPreset::Low:      return "low";
        default:                      return "very_high";
    }
}
static QualityPreset QualityPresetFromStr(const std::string& s) {
    if (s == "ultra")  return QualityPreset::Ultra;
    if (s == "high")   return QualityPreset::High;
    if (s == "medium") return QualityPreset::Medium;
    if (s == "low")    return QualityPreset::Low;
    return QualityPreset::VeryHigh;
}

static std::string BitrateModeToStr(BitrateMode v) {
    switch (v) {
        case BitrateMode::QP:  return "qp";
        case BitrateMode::VBR: return "vbr";
        case BitrateMode::CBR: return "cbr";
        default:               return "auto";
    }
}
static BitrateMode BitrateModeFromStr(const std::string& s) {
    if (s == "qp")  return BitrateMode::QP;
    if (s == "vbr") return BitrateMode::VBR;
    if (s == "cbr") return BitrateMode::CBR;
    return BitrateMode::Auto;
}

static std::string ReplayStorageToStr(ReplayStorage v) {
    return (v == ReplayStorage::Disk) ? "disk" : "ram";
}
static ReplayStorage ReplayStorageFromStr(const std::string& s) {
    return (s == "disk") ? ReplayStorage::Disk : ReplayStorage::RAM;
}

static std::string ContainerFormatToStr(ContainerFormat v) {
    switch (v) {
        case ContainerFormat::MKV: return "mkv";
        case ContainerFormat::FLV: return "flv";
        default:                   return "mp4";
    }
}
static ContainerFormat ContainerFormatFromStr(const std::string& s) {
    if (s == "mkv") return ContainerFormat::MKV;
    if (s == "flv") return ContainerFormat::FLV;
    return ContainerFormat::MP4;
}

static std::string ColorRangeToStr(ColorRange v) {
    return (v == ColorRange::Full) ? "full" : "limited";
}
static ColorRange ColorRangeFromStr(const std::string& s) {
    return (s == "full") ? ColorRange::Full : ColorRange::Limited;
}

static std::string FramerateModeToStr(FramerateMode v) {
    switch (v) {
        case FramerateMode::VFR:     return "vfr";
        case FramerateMode::Content: return "content";
        default:                     return "cfr";
    }
}
static FramerateMode FramerateModeFromStr(const std::string& s) {
    if (s == "vfr")     return FramerateMode::VFR;
    if (s == "content") return FramerateMode::Content;
    return FramerateMode::CFR;
}

static std::string TuneProfileToStr(TuneProfile v) {
    return (v == TuneProfile::Performance) ? "performance" : "quality";
}
static TuneProfile TuneProfileFromStr(const std::string& s) {
    return (s == "performance") ? TuneProfile::Performance : TuneProfile::Quality;
}

static std::string AudioModeToStr(AudioMode v) {
    if (v == AudioMode::Separated) return "separated";
    if (v == AudioMode::Virtual)   return "virtual";
    return "mixed";
}
static AudioMode AudioModeFromStr(const std::string& s) {
    if (s == "separated") return AudioMode::Separated;
    if (s == "virtual")   return AudioMode::Virtual;
    return AudioMode::Mixed;
}

// ─── InitializeOrCreateConfig ─────────────────────────────────────────────────
std::optional<Config> Config::InitializeOrCreateConfig() {
    Config settings;
    if (settings.Exists()) {
        if (settings.Load()) {
            std::cout << "[Config] Loaded existing settings" << std::endl;
            return settings;
        }
        std::cerr << "[Config] Corrupted, using defaults." << std::endl;
    } else {
        std::cout << "[Config] First run detected" << std::endl;
    }
    if (settings.Save()) {
        std::cout << "[Config] Created new config" << std::endl;
        return settings;
    }
    std::cerr << "[Config] FAILED to create Config!" << std::endl;
    return std::nullopt;
}

// ─── Load ─────────────────────────────────────────────────────────────────────
bool Config::Load() {
    const fs::path path = GetSettingsPath();
    if (!fs::exists(path)) return false;

    try {
        auto cfg = toml::parse_file(path.string());

        appVersion     = cfg["app"]["version"].value_or<std::string>("0.0.1-11022026");

        startMinimized = cfg["general"]["start_minimized"].value_or(false);
        libraryPath    = cfg["general"]["library_path"].value_or<std::string>("");

        recordingMode      = cfg["recording"]["mode"].value_or<std::string>("native");
        recordingAutoStart = cfg["recording"]["auto_start"].value_or(false);
        hotkeyRecordToggle = cfg["recording"]["hotkey_record_toggle"].value_or<std::string>("F10");
        hotkeySaveClip     = cfg["recording"]["hotkey_save_clip"].value_or<std::string>("F11");
        hotkeyToggleMic    = cfg["recording"]["hotkey_toggle_mic"].value_or<std::string>("F12");

        obsHost = cfg["recording"]["obs"]["host"].value_or<std::string>("localhost");
        obsPort = cfg["recording"]["obs"]["port"].value_or<int>(4455);

        // ── Native ──
        nativeScreenOutput = cfg["recording"]["native"]["screen_output"].value_or<std::string>("");

        nativeVideoCodec   = VideoCodecFromStr(cfg["recording"]["native"]["video_codec"].value_or<std::string>("h264"));
        nativeAudioCodec   = AudioCodecFromStr(cfg["recording"]["native"]["audio_codec"].value_or<std::string>("opus"));
        nativeEncoder      = EncoderModeFromStr(cfg["recording"]["native"]["encoder"].value_or<std::string>("gpu"));
        nativeFallbackCpu  = cfg["recording"]["native"]["fallback_cpu"].value_or(true);

        nativeQuality      = QualityPresetFromStr(cfg["recording"]["native"]["quality"].value_or<std::string>("very_high"));
        nativeBitrateMode  = BitrateModeFromStr(cfg["recording"]["native"]["bitrate_mode"].value_or<std::string>("vbr"));
        nativeVideoBitrate = cfg["recording"]["native"]["video_bitrate"].value_or<int>(5000);
        nativeAudioBitrate = cfg["recording"]["native"]["audio_bitrate"].value_or<int>(192);
        nativeFPS          = cfg["recording"]["native"]["fps"].value_or<int>(60);
        nativeClipDuration = cfg["recording"]["native"]["clip_duration"].value_or<int>(60);

        nativeReplayStorage   = ReplayStorageFromStr( cfg["recording"]["native"]["replay_storage"].value_or<std::string>("ram"));
        nativeShowCursor      = cfg["recording"]["native"]["show_cursor"].value_or(true);
        nativeContainerFormat = ContainerFormatFromStr(cfg["recording"]["native"]["container_format"].value_or<std::string>("mp4"));
        nativeColorRange      = ColorRangeFromStr(cfg["recording"]["native"]["color_range"].value_or<std::string>("limited"));
        nativeFramerateMode   = FramerateModeFromStr(cfg["recording"]["native"]["framerate_mode"].value_or<std::string>("vfr"));
        nativeTune            = TuneProfileFromStr(cfg["recording"]["native"]["tune"].value_or<std::string>("quality"));

        nativeAudioMode = AudioModeFromStr(cfg["recording"]["native"]["audio_mode"].value_or<std::string>("mixed"));

        if (const auto arr = cfg["recording"]["native"]["audio_tracks"].as_array()) {
            nativeAudioTracks.clear();
            for (auto& el : *arr) {
                if (!el.is_table()) continue;
                AudioTrack track;
                track.name       = (*el.as_table())["name"].value_or<std::string>("");
                track.device     = (*el.as_table())["device"].value_or<std::string>("default");
                const std::string typeStr = (*el.as_table())["device_type"].value_or<std::string>("input");
                track.deviceType = (typeStr == "output") ? AudioDeviceType::Output : AudioDeviceType::Input;
                if (!track.name.empty()) nativeAudioTracks.push_back(track);
            }
        }
        return true;

    } catch (const toml::parse_error& err) {
        std::cerr << "[Config] Parse error: " << err << std::endl;
        return false;
    }
}

// ─── Save ─────────────────────────────────────────────────────────────────────
bool Config::Save() const {
    try {
        const fs::path path = GetSettingsPath();
        fs::create_directories(path.parent_path());

        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "[Config] Cannot open: " << path << std::endl;
            return false;
        }

        file << "# ProjectMoment Settings — Auto-generated\n\n";

        file << "[app]\n";
        file << "version = \"" << appVersion << "\"\n\n";

        file << "[general]\n";
        file << "start_minimized = " << (startMinimized ? "true" : "false") << "\n";
        file << "library_path = \"" << libraryPath << "\"\n\n";

        file << "[recording]\n";
        file << "mode = \"" << recordingMode << "\"\n";
        file << "auto_start = " << (recordingAutoStart ? "true" : "false") << "\n";
        file << "hotkey_record_toggle = \"" << hotkeyRecordToggle << "\"\n";
        file << "hotkey_save_clip = \"" << hotkeySaveClip << "\"\n";
        file << "hotkey_toggle_mic = \"" << hotkeyToggleMic << "\"\n\n";

        file << "[recording.obs]\n";
        file << "host = \"" << obsHost << "\"\n";
        file << "port = " << obsPort << "\n\n";

        file << "[recording.native]\n";
        file << "screen_output = \"" << nativeScreenOutput << "\"\n";

        file << "video_codec = \""  << VideoCodecToStr(nativeVideoCodec)   << "\"\n";
        file << "audio_codec = \""  << AudioCodecToStr(nativeAudioCodec)   << "\"\n";
        file << "encoder = \""      << EncoderModeToStr(nativeEncoder)     << "\"\n";
        file << "fallback_cpu = "   << (nativeFallbackCpu ? "true" : "false") << "\n";

        file << "quality = \""      << QualityPresetToStr(nativeQuality)   << "\"\n";
        file << "bitrate_mode = \"" << BitrateModeToStr(nativeBitrateMode) << "\"\n";
        file << "video_bitrate = "  << nativeVideoBitrate << "\n";
        file << "audio_bitrate = "  << nativeAudioBitrate << "\n";
        file << "fps = "            << nativeFPS          << "\n";
        file << "clip_duration = "  << nativeClipDuration << "\n";

        file << "replay_storage = \""  << ReplayStorageToStr(nativeReplayStorage)   << "\"\n";
        file << "show_cursor = "       << (nativeShowCursor ? "true" : "false")     << "\n";
        file << "container_format = \"" << ContainerFormatToStr(nativeContainerFormat) << "\"\n";
        file << "color_range = \""     << ColorRangeToStr(nativeColorRange)         << "\"\n";
        file << "framerate_mode = \""  << FramerateModeToStr(nativeFramerateMode)   << "\"\n";
        file << "tune = \""            << TuneProfileToStr(nativeTune)              << "\"\n";
        file << "audio_mode = \""      << AudioModeToStr(nativeAudioMode)           << "\"\n\n";

        for (const auto& [name, device, deviceType] : nativeAudioTracks) {
            file << "[[recording.native.audio_tracks]]\n";
            file << "name = \""        << name   << "\"\n";
            file << "device = \""      << device << "\"\n";
            file << "device_type = \"" << (deviceType == AudioDeviceType::Output ? "output" : "input") << "\"\n\n";
        }

        std::cout << "[Config] Saved to: " << path << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[Config] Save failed: " << e.what() << std::endl;
        return false;
    }
}

bool Config::Exists() {
    return fs::exists(GetSettingsPath());
}

fs::path Config::GetSettingsPath() {
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"))
        return fs::path(appdata) / "projectMoment" / "config.toml";
#else
    if (const char* home = std::getenv("HOME"))
        return fs::path(home) / ".config" / "projectMoment" / "config.toml";
#endif
    return fs::current_path() / "config.toml";
}