#include "gui/widgets/FolderBrowser.h"

#include "core/CoreServices.h"

FolderBrowser::FolderBrowser()
    : m_fileDialog(ImGuiFileBrowserFlags_SelectDirectory |
                   ImGuiFileBrowserFlags_CreateNewDir)
    , m_title("Select Folder")
    , m_windowSize(800, 600)
    , m_shouldDraw(false)
    , m_selectionSuccess(false) {

    m_fileDialog.SetTitle(m_title);
    m_fileDialog.SetWindowSize(m_windowSize.x, m_windowSize.y);
}


/**
 * Window Lifecycle & Rendering
 **/
void FolderBrowser::Open() {
    m_fileDialog.Open();
    m_shouldDraw = true;
    m_selectionSuccess = false;
}

void FolderBrowser::Close() {
    m_fileDialog.Close();
    m_shouldDraw = false;

    if (m_onCancelled) {
        m_onCancelled();
    }
}


void FolderBrowser::Draw() {
    if (!m_shouldDraw) {
        return;
    }

    DrawDimBackground();
    m_fileDialog.Display();

    // Check if user selected a folder
    if (m_fileDialog.HasSelected()) {
        m_currentPath = m_fileDialog.GetSelected();
        m_selectionSuccess = true;
        m_shouldDraw = false;

        std::cout << "[FolderBrowser] Selected path: " << m_currentPath << std::endl;

        // Update config with selected path
        auto& services = CoreServices::Instance();

        if (auto* config = services.GetConfig()) {
            config->libraryPath = m_currentPath.string();
            config->Set("library", "path", m_currentPath.string());
            std::cout << "[FolderBrowser] Config updated successfully" << std::endl;
        }

        // Fire selection callback
        if (m_onSelected) {
            m_onSelected(m_currentPath);
        }
    }

    // Close dialog if it was closed by user
    if (!m_fileDialog.IsOpened() && m_shouldDraw) {
        m_shouldDraw = false;
    }
}

/**
 * Event callbacks
 **/
void FolderBrowser::SetOnFolderSelected(const FolderSelectedCallback &callback) {
    m_onSelected = callback;
}

void FolderBrowser::SetOnCancelled(const FolderCancelledCallback &callback) {
    m_onCancelled = callback;
}


/**
 * Configuration & Settings
 **/
void FolderBrowser::SetTitle(const std::string& title) {
    m_title = title;
    m_fileDialog.SetTitle(title);
}

void FolderBrowser::SetInitialPath(const fs::path& path) {
    if (fs::exists(path) && fs::is_directory(path)) {
        m_fileDialog.SetPwd(path);
    }
}

void FolderBrowser::SetWindowSize(const int width, const int height) {
    m_windowSize = ImVec2(width, height);
    m_fileDialog.SetWindowSize(width, height);
}


/**
 * State queries
 **/
bool FolderBrowser::IsOpen() const noexcept {
    return m_shouldDraw;
}

bool FolderBrowser::HasSelected() const noexcept {
    return m_selectionSuccess;
}

/**
 * Selection Data
 **/
fs::path FolderBrowser::GetSelectedPath() const {
    return m_currentPath;
}

void FolderBrowser::ClearSelection() {
    m_selectionSuccess = false;
    m_currentPath.clear();
}

/**
 * Window Configuration
 **/
void FolderBrowser::DrawDimBackground() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    // ImGui-FileBrowser flags
    constexpr ImGuiWindowFlags bgFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.7f));

    if (ImGui::Begin("##DimBackground", nullptr, bgFlags)) {
        ImGui::End();
    }

    ImGui::PopStyleColor();
}