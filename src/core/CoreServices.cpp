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
    if (m_initialized) return;

    if (!m_config || m_config->libraryPath.empty()) {
        return;
    }

    try {
        m_paths = ProjectPaths::FromFolder(m_config->libraryPath);

        m_videoLibrary = std::make_unique<VideoLibrary>(m_paths);
        m_videoImportService = std::make_unique<VideoImportService>();
        m_videoDatabase = std::make_unique<VideoDatabase>(m_paths.dbPath.string());

        m_initialized = true;
        std::cout << "[CoreServices] All services initialized successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[CoreServices] Critical initialization error: " << e.what() << std::endl;
        Shutdown();
    }
}

bool CoreServices::IsInitialized() const {
    std::lock_guard lock(m_mutex);
    return m_initialized;
}



void CoreServices::Shutdown() {
    std::lock_guard lock(m_mutex);
    
    std::cout << "[CoreServices] Shutting down..." << std::endl;

    if (m_videoLibrary) {
        m_videoLibrary.reset();
    }

    if (m_videoDatabase) {
        m_videoDatabase.reset();
    }

    if (m_videoImportService) {
        m_videoImportService.reset();
    }

    if (m_recordingManager) {
        if (m_recordingManager->IsRecording()) {
            m_recordingManager->StopRecording();
        }
        m_recordingManager.reset();
    }

    m_initialized = false;
    
    std::cout << "[CoreServices] Shutdown complete" << std::endl;
}