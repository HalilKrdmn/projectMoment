#include "core/Config.h"

#include <fstream>

// Create or load when started.
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



bool Config::Load() {
    const fs::path path = GetSettingsPath();

    if (!fs::exists(path)) {
        return false;
    }

    try {
        auto AppSettings = toml::parse_file(path.string());

        appVersion     = AppSettings["app"]["version"].value_or<std::string>("0.0.1-11022026");

        // ─── GENERAL SETTINGS ───────────────────────────────────────────────────────

        startMinimized = AppSettings["general"]["start_minimized"].value_or(false);
        libraryPath    = AppSettings["general"]["library_path"].value_or<std::string>("");

        // ─── RECORDING SETTINGS ─────────────────────────────────────────────────────

        recordingMode      = AppSettings["recording"]["mode"].value_or<std::string>("native");
        recordingAutoStart = AppSettings["recording"]["auto_start"].value_or(false);
        hotkeyRecordToggle = AppSettings["recording"]["hotkey_record_toggle"].value_or<std::string>("F10");
        hotkeySaveClip     = AppSettings["recording"]["hotkey_save_clip"].value_or<std::string>("F11");
        hotkeyToggleMic    = AppSettings["recording"]["hotkey_toggle_mic"].value_or<std::string>("F12");

        obsHost = AppSettings["recording"]["obs"]["host"].value_or<std::string>("localhost");
        obsPort = AppSettings["recording"]["obs"]["port"].value_or<int>(4455);

        // Audio mode
        if (const std::string audioModeStr = AppSettings["recording"]["native"]["audio_mode"].value_or<std::string>("mixed"); audioModeStr == "separated")
                                            nativeAudioMode = AudioMode::Separated;
        else if (audioModeStr == "virtual") nativeAudioMode = AudioMode::Virtual;
        else                                nativeAudioMode = AudioMode::Mixed;
        // Audio tracks
        if (const auto arr = AppSettings["recording"]["native"]["audio_tracks"].as_array()) {
            nativeAudioTracks.clear();
            for (auto& el : *arr) {
                if (!el.is_table()) continue;
                AudioTrack track;
                track.name   = (*el.as_table())["name"].value_or<std::string>("");
                track.device = (*el.as_table())["device"].value_or<std::string>("default");
                const std::string typeStr = (*el.as_table())["device_type"].value_or<std::string>("input");
                track.deviceType = (typeStr == "output") ? AudioDeviceType::Output : AudioDeviceType::Input;
                if (!track.name.empty()) nativeAudioTracks.push_back(track);
            }
        }
        nativeScreenOutput = AppSettings["recording"]["native"]["screen_output"].value_or<std::string>(std::move(nativeScreenOutput));
        nativeVideoCodec = AppSettings["recording"]["native"]["video_codec"].value_or<std::string>(std::move(nativeVideoCodec));
        nativeAudioCodec = AppSettings["recording"]["native"]["audio_codec"].value_or<std::string>(std::move(nativeAudioCodec));
        nativeVideoBitrate = AppSettings["recording"]["native"]["video_bitrate"].value_or<int>(std::move(nativeVideoBitrate));
        nativeAudioBitrate = AppSettings["recording"]["native"]["audio_bitrate"].value_or<int>(std::move(nativeAudioBitrate));
        nativeFPS = AppSettings["recording"]["native"]["fps"].value_or<int>(std::move(nativeFPS));
        replayBufferDuration = AppSettings["recording"]["native"]["replay_buffer_duration"].value_or<int>(std::move(replayBufferDuration));

        // ──────────────────────────────────────────────────────────────────────────
        return true;

    } catch (const toml::parse_error& err) {
        std::cerr << "[Config] Parse error: " << err << std::endl;
        return false;
    }
}

bool Config::Save() const {
    try {
        const fs::path path = GetSettingsPath();

        // Create Config directory
        fs::create_directories(path.parent_path());

        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "[Config] Cannot open: " << path << std::endl;
            return false;
        }

        file << "#\n";
        file << "# ProjectMoment Settings\n";
        file << "# Auto-generated - Manual edits are preserved\n";
        file << "#";
        file << "# DON'T TOUCH!";
        file << "[app]\n";
        file << "version = \"" << appVersion << "\"\n";
        file << "#";
        file << "\n";
        file << "\n";


        file << "# GENERAL SETTINGS\n";
        file << "[general]\n";
        file << "start_minimized = " << (startMinimized ? "true" : "false") << "\n";
        file << "library_path = \"" << libraryPath << "\"\n\n";
        file << "\n";

        file << "# RECORDING SETTINGS\n";

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
        // Audio mode
        std::string audioModeStr = "mixed";
        if (nativeAudioMode == AudioMode::Separated) audioModeStr = "separated";
        else if (nativeAudioMode == AudioMode::Virtual) audioModeStr = "virtual";
        file << "audio_mode = \"" << audioModeStr << "\"\n\n";
        file << "screen_output = \"" << nativeScreenOutput << "\"\n";
        file << "video_codec = \"" << nativeVideoCodec << "\"\n";
        file << "audio_codec = \"" << nativeAudioCodec << "\"\n";
        file << "video_bitrate = " << nativeVideoBitrate << "\n";
        file << "audio_bitrate = " << nativeAudioBitrate << "\n";
        file << "fps = " << nativeFPS << "\n";
        file << "replay_buffer_duration = " << replayBufferDuration << "\n";
        file << "\n";
        // Audio tracks
        for (const auto&[name, device, deviceType] : nativeAudioTracks) {
            file << "[[recording.native.audio_tracks]]\n";
            file << "name = \"" << name << "\"\n";
            file << "device = \"" << device << "\"\n";
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
        // NOT TESTED!
        if (const char* appdata = std::getenv("APPDATA");) {
            return fs::path(appdata) / "projectMoment" / "config.toml";
        }
    #else
        if (const char* home = std::getenv("HOME")) {
            return fs::path(home) / ".config" / "projectMoment" / "config.toml";
        }
    #endif

    return fs::current_path() / "config.toml";
}