#include "gui/screens/main/MainScreen.h"

#include "gui/Theme.h"
#include "core/CoreServices.h"
#include "core/library/LibraryLoader.h"
#include "core/library/VideoLibrary.h"

#include "imgui.h"

#include <cmath>
#include <filesystem>
#include <sys/statvfs.h>
#include <iostream>
#include <thread>

MainScreen::MainScreen(MainWindow* manager)
    : BaseScreen(manager){

    // Create states
    m_welcomeState = std::make_unique<WelcomeState>();
    m_loadingState = std::make_unique<LoadingState>();
    m_videoListState = std::make_unique<VideoListState>();
    m_emptyFolderState = std::make_unique<EmptyFolderState>();

    m_onSettingsClicked = [this]() {
        GetManager()->SetApplicationState(ApplicationState::SETTINGS);
    };

    DetermineInitialState();
}

std::filesystem::path MainScreen::GetCurrentFolder() const {
    const auto* config = CoreServices::Instance().GetConfig();
    if (config && !config->libraryPath.empty())
        return std::filesystem::path(config->libraryPath);
    return {};
}

void MainScreen::DetermineInitialState() {
    // Each time the screen appears, it checks the path in the config.
    // If no path is specified in the config, the screen is set to WelcomeState.
    if (!ValidateLibraryPath()) {
        ChangeState(MainScreenState::WELCOME);
        return;
    }

    if (auto* recMgr = CoreServices::Instance().GetRecordingManager()) {
        recMgr->SetOnClipSaved([this](const fs::path&) {
            RefreshLibrarySilent();
        });
    }

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

    std::cout << "[MainScreen] Config path: '" << config->libraryPath << "'\n";

    // If the file path in the config is empty, it displays the WelcomeStates screen.
    if (config->libraryPath.empty()) {
        std::cout << "[MainScreen] Path is empty, switching to WELCOME\n";
        return false;
    }

    // If the file path exists in the config but the file is not on the disk,
    // it displays the WelcomeStates screen directly without creating a folder.
    if (!std::filesystem::exists(config->libraryPath)) {
        std::cout << "[MainScreen] Path in config does not exist on disk, switching to WELCOME\n";
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
        std::cerr << "[Error] VideoLibrary could not be initialized!\n";
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
        std::cout << "[MainScreen] Loaded " << videos.size() << " videos\n";
    }).detach();
}

void MainScreen::RefreshLibrarySilent() {
    auto& services     = CoreServices::Instance();
    auto* library      = services.GetVideoLibrary();
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

// ─── Draw ─────────────────────────────────────────────────────────────────
void MainScreen::Draw() {
    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize     |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin(GetCurrentWindowName(), nullptr, flags);

    if (m_currentState == MainScreenState::VIDEO_LIST ||
        m_currentState == MainScreenState::EMPTY_FOLDER)
    {
        DrawTopBar();
        ImGui::Spacing();
    }

    switch (m_currentState) {
        case MainScreenState::WELCOME:      m_welcomeState->Draw(this);     break;
        case MainScreenState::LOADING:      m_loadingState->Draw(this);     break;
        case MainScreenState::VIDEO_LIST:   m_videoListState->Draw(this);   break;
        case MainScreenState::EMPTY_FOLDER: m_emptyFolderState->Draw(this); break;
    }

    ImGui::End();
    m_folderBrowser.Draw();
}

const char* MainScreen::GetCurrentWindowName() const {
    switch (m_currentState) {
        case MainScreenState::WELCOME:      return "Welcome";
        case MainScreenState::LOADING:      return "Loading";
        case MainScreenState::VIDEO_LIST:   return "VideoList";
        case MainScreenState::EMPTY_FOLDER: return "EmptyFolder";
        default:                            return "MainScreen";
    }
}

// ─── Top Bar ────────────────────────────────────────────────────────────────
void MainScreen::DrawTopBar() {
    constexpr float barHeight = 40.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::BeginChild("TopBar", ImVec2(0, barHeight), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleColor();

    const auto* config      = CoreServices::Instance().GetConfig();
    const StorageInfo info  = CalculateStorageInfo(config->libraryPath, m_currentVideos.size());

    ImGui::BeginGroup();
    DrawStorageInfo(info);
    ImGui::EndGroup();

    constexpr float recBtnW      = 155.0f;
    constexpr float clipBtnW     = 120.0f;
    constexpr float settingsBtnW = 40.0f;
    constexpr float gap          = 8.0f;
    constexpr float edgePad      = 20.0f;
    constexpr float rightW = recBtnW + gap + clipBtnW + gap + settingsBtnW + edgePad;

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - rightW
                    + ImGui::GetCursorPosX() - ImGui::GetScrollX());

    DrawRecordToggleButton();
    ImGui::SameLine(0, gap);
    DrawClipSaveButton();
    ImGui::SameLine(0, gap);

    ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::ACCENT_ACTIVE);
    if (ImGui::Button("S", ImVec2(settingsBtnW, 40.0f)))
        if (m_onSettingsClicked) m_onSettingsClicked();
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
}

void MainScreen::DrawRecordToggleButton() {
    auto* recMgr          = CoreServices::Instance().GetRecordingManager();
    const bool isRecording = recMgr && recMgr->IsRecording();

    const ImVec2  pos  = ImGui::GetCursorScreenPos();
    constexpr ImVec2 size = { 155.0f, 40.0f };
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (isRecording) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.12f, 0.10f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.14f, 0.14f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.08f, 0.06f, 0.06f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BTN_HOVER);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::BTN_ACTIVE);
    }

    if (ImGui::Button("##RecBtn", size)) {
        if (recMgr) {
            if (isRecording) recMgr->StopRecording();
            else             recMgr->StartRecording();
        }
    }
    ImGui::PopStyleColor(3);

    constexpr float radius = 5.0f;
    const ImVec2 dotPos = { pos.x + 18.0f, pos.y + size.y * 0.5f };

    if (isRecording) {
        const float t = static_cast<float>(ImGui::GetTime());
        for (int i = 0; i < 2; ++i) {
            const float wave  = fmodf(t + i * 0.8f, 1.5f) / 1.5f;
            const float wR    = radius + wave * 10.0f;
            const float alpha = 1.0f - wave;
            dl->PushClipRect(pos, { pos.x + size.x, pos.y + size.y }, true);
            dl->AddCircle(dotPos, wR, ImColor(1.0f, 0.25f, 0.25f, alpha * 0.5f), 24, 1.5f);
            dl->PopClipRect();
        }
        dl->AddCircleFilled(dotPos, radius, ImColor(1.0f, 0.22f, 0.22f, 1.0f), 24);
    } else {
        dl->AddCircleFilled(dotPos, radius, ImGui::GetColorU32(Theme::TEXT_MUTED), 24);
    }

    const char*  label = isRecording ? "STOP RECORDING" : "START RECORDING";
    const ImVec2 ts    = ImGui::CalcTextSize(label);
    dl->AddText({ pos.x + 32.0f, pos.y + (size.y - ts.y) * 0.5f },
                ImGui::GetColorU32(Theme::TEXT_PRIMARY), label);
}

void MainScreen::DrawClipSaveButton() {
    auto* recMgr          = CoreServices::Instance().GetRecordingManager();
    const bool isRecording = recMgr && recMgr->IsRecording();
    const bool isSaving    = recMgr && recMgr->IsSavingClip();

    ImGui::BeginDisabled(!recMgr || isSaving);

    if (isSaving) {
        ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BTN_NEUTRAL);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::BTN_NEUTRAL);
        ImGui::PushStyleColor(ImGuiCol_Text,          Theme::TEXT_MUTED);
        ImGui::Button("Processing...", ImVec2(120.0f, 40.0f));
        ImGui::PopStyleColor(4);
    } else {
        if (isRecording) {
            ImGui::PushStyleColor(ImGuiCol_Button,        Theme::ACCENT);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT_HOVER);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::ACCENT_ACTIVE);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::BTN_HOVER);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::BTN_ACTIVE);
        }
        if (ImGui::Button("SAVE CLIP", ImVec2(120.0f, 40.0f)))
            recMgr->SaveClip();
        ImGui::PopStyleColor(3);
    }

    ImGui::EndDisabled();
}

void MainScreen::DrawStorageInfo(const StorageInfo& info) {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);

    ImGui::Text("%zu VIDEOS", info.totalVideos);

    ImGui::SameLine(0, 16);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::SEPARATOR);
    ImGui::TextUnformatted("|");
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 16);
    ImGui::Text("%.1f GB USED", info.usedSpaceGB);

    ImGui::SameLine(0, 16);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::SEPARATOR);
    ImGui::TextUnformatted("|");
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 16);
    const float safeTotal = std::max(info.totalSpaceGB, 0.001f);
    const bool  critical  = (info.usedSpaceGB / safeTotal) > 0.9f;
    ImGui::PushStyleColor(ImGuiCol_Text, critical ? Theme::DANGER : Theme::SUCCESS);
    ImGui::Text("%.1f GB FREE", info.freeSpaceGB);
    ImGui::PopStyleColor();
}

StorageInfo MainScreen::CalculateStorageInfo(const std::string& libraryPath, size_t videoCount) {
    StorageInfo info;
    info.totalVideos = videoCount;
    if (libraryPath.empty() || !std::filesystem::exists(libraryPath)) return info;

    struct statvfs stat;
    if (statvfs(libraryPath.c_str(), &stat) == 0) {
        constexpr double gb   = 1024.0 * 1024.0 * 1024.0;
        info.totalSpaceGB = static_cast<float>((stat.f_blocks * static_cast<double>(stat.f_frsize)) / gb);
        info.freeSpaceGB  = static_cast<float>((stat.f_bfree  * static_cast<double>(stat.f_frsize)) / gb);
        info.usedSpaceGB  = info.totalSpaceGB - info.freeSpaceGB;
    }
    return info;
}