#include "core/CoreServices.h"

CoreServices::CoreServices() {
    if (auto loadedConfig = Config::InitializeOrCreateConfig(); loadedConfig.has_value()) {
        m_config = std::make_unique<Config>(loadedConfig.value());
    } else {
        m_config = std::make_unique<Config>();
    }
}

CoreServices& CoreServices::Instance() {
    static CoreServices instance;
    return instance;
}

void CoreServices::Initialize() {
    std::lock_guard lock(m_mutex);
    InitializeInternal();
}

void CoreServices::InitializeInternal() {
    if (m_initialized) return;

    try {
        if (!m_config) {
            const auto loadedConfig = Config::InitializeOrCreateConfig();
            m_config = std::make_unique<Config>(loadedConfig.value_or(Config()));
        }

        if (m_config->libraryPath.empty()) return;

        m_paths = ProjectPaths::FromFolder(m_config->libraryPath);

        m_videoDatabase = std::make_unique<VideoDatabase>(m_paths.dbPath.string());
        m_videoLibrary = std::make_unique<VideoLibrary>(m_paths);
        m_videoImportService = std::make_unique<VideoImportService>();

        m_initialized = true;
        std::cout << "[CoreServices] All services initialized successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[CoreServices] Init Error: " << e.what() << std::endl;
    }
}

void CoreServices::Shutdown() {
    std::lock_guard lock(m_mutex);

    if (!m_initialized && !m_config) return;

    std::cout << "[CoreServices] Shutting down..." << std::endl;

    if (m_recordingManager) {
        std::cout << "[CoreServices] Stopping Recording Manager..." << std::endl;
        m_recordingManager.reset();
    }

    if (m_videoLibrary) {
        std::cout << "[CoreServices] Stopping Video Library..." << std::endl;
        m_videoLibrary.reset();
    }

    if (m_videoImportService) {
        m_videoImportService.reset();
    }

    if (m_videoDatabase) {
        std::cout << "[CoreServices] Closing Database..." << std::endl;
        m_videoDatabase.reset();
    }

    if (m_config) {
        m_config.reset();
    }

    m_initialized = false;
    std::cout << "[CoreServices] Shutdown complete" << std::endl;
}