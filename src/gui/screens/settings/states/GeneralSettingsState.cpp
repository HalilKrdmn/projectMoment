#include "gui/screens/settings/states/GeneralSettingsState.h"

#include "gui/Theme.h"
#include "core/CoreServices.h"

#include <cstring>
#include <filesystem>

GeneralSettingsState::GeneralSettingsState() {
    LoadFromConfig();
}

void GeneralSettingsState::LoadFromConfig() {
    const Config* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    std::strncpy(m_libraryPath, cfg->libraryPath.c_str(), sizeof(m_libraryPath) - 1);
    m_autoStartBuffer = cfg->recordingAutoStart;
    m_startMinimized  = cfg->startMinimized;

    SyncOriginals();
    m_dirty = false;
}

// ─── Dirty tracking ───────────────────────────────────────────────────────
void GeneralSettingsState::SyncOriginals() {
    std::strncpy(m_origLibraryPath, m_libraryPath, sizeof(m_origLibraryPath) - 1);
    m_origAutoStartBuffer = m_autoStartBuffer;
    m_origStartMinimized  = m_startMinimized;
}

void GeneralSettingsState::CheckDirty() {
    m_dirty = (std::strcmp(m_libraryPath, m_origLibraryPath) != 0)
           || (m_autoStartBuffer != m_origAutoStartBuffer)
           || (m_startMinimized  != m_origStartMinimized);
}

// ─── Draw ──────────────────────────────────────────────────────────────────
void GeneralSettingsState::Draw() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BG_CONTENT);
    ImGui::BeginChild("##general_content", ImVec2(0,0), false);

    constexpr float labelW = 200.0f;
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("General");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Text("Log Directory");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(280.0f);
    if (ImGui::InputText("##lib", m_libraryPath, sizeof(m_libraryPath)))
        CheckDirty();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT_HOVER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::ACCENT_ACTIVE);
    if (ImGui::SmallButton("  Browse  ")) { m_folderBrowser.Open(); } // TODO: NOT TESTED
    ImGui::PopStyleColor(3);

    ImGui::SameLine(0, 10);
    if (std::strlen(m_libraryPath) > 0) {
        if (std::filesystem::exists(m_libraryPath)) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f,0.8f,0.3f,1.0f));
            ImGui::TextUnformatted("OK");
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::DANGER);
            ImGui::TextUnformatted("FAIL");
        }
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("BEGINNING");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Text("Automatically start buffer");
    ImGui::SameLine(labelW);
    if (ImGui::Checkbox("##auto_buf", &m_autoStartBuffer)) CheckDirty();
    ImGui::SameLine(0, 12);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("The buffer starts immediately when the application opens.");
    ImGui::PopStyleColor();

    ImGui::Text("Start with a Shrink to Tray");
    ImGui::SameLine(labelW);
    if (ImGui::Checkbox("##tray_start", &m_startMinimized)) CheckDirty();
    ImGui::SameLine(0, 12);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("The application runs in the background without opening a window.");
    ImGui::PopStyleColor();

    m_folderBrowser.Draw();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ─── Save ──────────────────────────────────────────────────────────────────────
void GeneralSettingsState::SaveChanges() const {
    Config* cfg = CoreServices::Instance().GetConfig();

    cfg->libraryPath       = m_libraryPath;
    cfg->recordingAutoStart = m_autoStartBuffer;
    cfg->startMinimized    = m_startMinimized;

    if (!cfg->Save()) {
        std::cerr << "[Settings/GeneralSettingsState] Config save failed" << std::endl;
    } else {
        std::cout << "[Settings/GeneralSettingsState] Config updated successfully" << std::endl;
    }
}