#pragma once

#include "core/ProjectPaths.h"

#include "core/Config.h"
#include "core/library/VideoLibrary.h"
#include "core/library/VideoDatabase.h"
#include "core/import/VideoImportService.h"
#include "core/recording/RecordingManager.h"

#include <memory>

class CoreServices {
public:
    static CoreServices& Instance();

    RecordingManager* GetRecordingManager() {
        std::lock_guard lock(m_mutex);

        if (!m_initialized) { InitializeInternal(); }
        if (!m_recordingManager) {
            m_recordingManager = std::make_unique<RecordingManager>();
            m_recordingManager->Initialize();
        }
        return m_recordingManager.get();
    }

    Config* GetConfig() {
        return GetService(m_config);
    }

    VideoLibrary* GetVideoLibrary() {
        return GetService(m_videoLibrary);
    }

    VideoDatabase* GetVideoDatabase() {
        return GetService(m_videoDatabase);
    }

    VideoImportService* GetVideoImportService() {
        return GetService(m_videoImportService);
    }

    void Initialize();
    void Shutdown();

private:
    CoreServices();
    void InitializeInternal();

    // Template method
    template<typename T>
    T* GetService(std::unique_ptr<T>& service) {
        std::lock_guard lock(m_mutex);
        if (!m_initialized) InitializeInternal();
        return service.get();
    }

    // Member variables
    std::unique_ptr<Config> m_config;
    std::unique_ptr<VideoLibrary> m_videoLibrary;
    std::unique_ptr<VideoDatabase> m_videoDatabase;
    std::unique_ptr<VideoImportService> m_videoImportService;
    std::unique_ptr<RecordingManager> m_recordingManager;

    std::recursive_mutex m_mutex;
    bool m_initialized = false;
    ProjectPaths m_paths;
};
