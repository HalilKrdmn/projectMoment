#pragma once

#include "core/ProjectPaths.h"

#include "core/Config.h"
#include "core/library/VideoLibrary.h"
#include "core/library/VideoDatabase.h"
#include "core/import/VideoImportService.h"

#include <memory>

class CoreServices {
public:
    static CoreServices& Instance();

    // Public accessors
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

    bool IsInitialized() const;
    void Shutdown();

private:
    CoreServices();
    void Initialize();

    // Template method
    template<typename T>
    T* GetService(std::unique_ptr<T>& service) {
        Initialize();
        std::lock_guard lock(m_mutex);
        return service.get();
    }

    // Member variables
    std::unique_ptr<Config> m_config;
    std::unique_ptr<VideoLibrary> m_videoLibrary;
    std::unique_ptr<VideoDatabase> m_videoDatabase;
    std::unique_ptr<VideoImportService> m_videoImportService;

    mutable std::mutex m_mutex;
    bool m_initialized = false;
    ProjectPaths m_paths;
};
