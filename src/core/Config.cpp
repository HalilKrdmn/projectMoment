#include "core/Config.h"
#include <fstream>

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

        // App info
        appVersion = AppSettings["app"]["version"].value_or<std::string>(std::move(appVersion));
        appName = AppSettings["app"]["name"].value_or<std::string>(std::move(appName));

        // Library
        libraryPath = AppSettings["library"]["path"].value_or<std::string>("");



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
        file << "#\n";
        file << "\n";

        // [app]
        file << "[app]\n";
        file << "version = \"" << appVersion << "\"\n";
        file << "name = \"" << appName << "\"\n";
        file << "\n";

        // [library]
        file << "[library]\n";
        file << "path = \"" << libraryPath << "\"\n";
        file << "\n";

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