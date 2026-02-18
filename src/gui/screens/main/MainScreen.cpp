#include "gui/screens/main/MainScreen.h"

#include "gui/screens/main/states/WelcomeState.h"
#include "gui/screens/main/states/LoadingState.h"
#include "gui/screens/main/states/VideoListState.h"
#include "gui/screens/main/states/EmptyFolderState.h"

#include "core/library/LibraryLoader.h"
#include "core/library/VideoLibrary.h"
#include "core/import/VideoImportService.h"

MainScreen::MainScreen(MainWindow* manager)
    : BaseScreen(manager){

    // Create states
    m_welcomeState = std::make_unique<WelcomeState>();
    m_loadingState = std::make_unique<LoadingState>();
    m_videoListState = std::make_unique<VideoListState>();
    m_emptyFolderState = std::make_unique<EmptyFolderState>();

    m_topBar = std::make_unique<TopBar>();

    m_topBar->SetOnRecordClicked([this]() {
        if (!m_recordingManager) {
            printf("[MainScreen] Lazy initializing RecordingManager\n");
            m_recordingManager = std::make_unique<RecordingManager>();
            m_recordingManager->Initialize();
        }

        if (m_recordingManager->IsRecording()) {
            m_recordingManager->StopRecording();
            RefreshLibrarySilent();
            printf("[MainScreen] Recording stopped\n");
        } else {
            const bool success = m_recordingManager->StartRecording();
            printf("[MainScreen] Recording started: %s\n", success ? "success" : "failed");
        }
    });

    m_topBar->SetOnSettingsClicked([this]() {
        GetManager()->SetApplicationState(ApplicationState::SETTINGS);
    });

    DetermineInitialState();
}

// It checks every time the screen turns on.
void MainScreen::DetermineInitialState() {
    // Each time the screen appears, it checks the path in the config.
    // If no path is specified in the config, the screen is set to WelcomeState.
    if (!ValidateLibraryPath()) {
        ChangeState(MainScreenState::WELCOME);
        return;
    }

    // If the file path is found, it is scanned to check for any changes.
    StartLibraryLoad();
}

// Checks the file path in the config file.
bool MainScreen::ValidateLibraryPath() {
    // Starts the config service
    auto& services = CoreServices::Instance();
    const auto* config = services.GetConfig();

    if (!config) {
        printf("[MainScreen] ValidateLibraryPath: ERROR - Config is NULL!\n");
        return false;
    }

    std::cout << "[Debug] Config path: '" << config->libraryPath << "'" << std::endl;

    // If the file path in the config is empty, it displays the WelcomeStates screen.
    if (config->libraryPath.empty()) {
        std::cout << "[Debug] Path is empty, switching to WELCOME" << std::endl;
        return false;
    }

    // If the file path exists in the config but the file is not on the disk,
    // it displays the WelcomeStates screen directly without creating a folder.
    if (!std::filesystem::exists(config->libraryPath)) {
        std::cout << "[Debug] Path in config does not exist on disk, switching to WELCOME" << std::endl;
        return false;
    }

    // If everything is okay, return to true.
    return true;
}

// Loads the folder in the selected path.
void MainScreen::StartLibraryLoad() {
    // Starts the config and Library services
    auto& services = CoreServices::Instance();
    auto* library = services.GetVideoLibrary();
    const auto* config = services.GetConfig();

    if (!library && !config) {
        std::cerr << "[Error] VideoLibrary could not be initialized!" << std::endl;
        ChangeState(MainScreenState::WELCOME);
        return;
    }

    // Clears the loading screen and prepares for loading.
    // (If there is nothing to load, the loading screen will not appear???)
    m_loadingState->Clear();
    ChangeState(MainScreenState::LOADING);

    // It takes the file path from the library path section in the config and moves it to the Loading screen.
    std::thread([this, library, config]() {
        library->CleanupOrphanedRecords();

        LibraryLoader::Run(library, config->libraryPath,
            [this](const std::string& msg, const float progress) {
                m_loadingState->AddLog(msg, progress);
                m_loadingState->SetProgress(progress);
            }
        );

        // It scans and records videos in the folder. If they are in the database, it skips them.
        const auto videos = library->GetAllVideos();
        this->SetCurrentVideos(videos);

        ChangeState(videos.empty() ? MainScreenState::EMPTY_FOLDER : MainScreenState::VIDEO_LIST);
        std::cout << "[MainScreen] Loaded " << videos.size() << " videos" << std::endl;
    }).detach();
}

void MainScreen::RefreshLibrarySilent() {
    auto& services = CoreServices::Instance();
    auto* library = services.GetVideoLibrary();
    const auto* config = services.GetConfig();

    if (!library || !config) return;

    std::thread([this, library, config]() {
        LibraryLoader::Run(library, config->libraryPath, nullptr);

        const auto videos = library->GetAllVideos();
        this->SetCurrentVideos(videos);

        m_videoListState->RequestThumbnailReload();
        ChangeState(videos.empty() ? MainScreenState::EMPTY_FOLDER : MainScreenState::VIDEO_LIST);
    }).detach();
}

// Rendering Methods
void MainScreen::Draw() {
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin(GetCurrentWindowName(), nullptr, flags);


    if (m_currentState == MainScreenState::VIDEO_LIST ||
        m_currentState == MainScreenState::EMPTY_FOLDER) {

        m_topBar->Draw(this, m_recordingManager.get());
        ImGui::Spacing();
        }

    switch (m_currentState) {
        case MainScreenState::WELCOME:
            m_welcomeState->Draw(this);
            break;
        case MainScreenState::LOADING:
            m_loadingState->Draw(this);
            break;
        case MainScreenState::VIDEO_LIST:
            m_videoListState->Draw(this);
            break;
        case MainScreenState::EMPTY_FOLDER:
            m_emptyFolderState->Draw(this);
            break;
    }

    ImGui::End();
    m_folderBrowser.Draw();
}

const char* MainScreen::GetCurrentWindowName() const {
    switch (m_currentState) {
        case MainScreenState::WELCOME:       return "Welcome";
        case MainScreenState::LOADING:       return "Loading";
        case MainScreenState::VIDEO_LIST:    return "VideoList";
        case MainScreenState::EMPTY_FOLDER:  return "EmptyFolder";
        default:                          return "MainScreen";
    }
}