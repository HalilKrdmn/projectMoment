#include "gui/screens/settings/states/OBSSettingsState.h"

#include "core/CoreServices.h"
#include "core/Config.h"
#include "imgui.h"

#include <cstring>
#include <algorithm>

OBSSettingsState::OBSSettingsState() {
    const auto* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    std::strncpy(m_host, cfg->obsHost.c_str(), sizeof(m_host) - 1);
    m_port = cfg->obsPort;
    SyncOriginals();
    m_dirty = false;
}

// ─── Dirty tracking ───────────────────────────────────────────────────────
void OBSSettingsState::SyncOriginals() {
    std::strncpy(m_origHost, m_host, sizeof(m_origHost) - 1);
    m_origPort = m_port;
}

void OBSSettingsState::CheckDirty() {
    m_dirty = (std::strcmp(m_host, m_origHost) != 0)
           || (m_port != m_origPort);
}

// ─── Draw ──────────────────────────────────────────────────────────────────
void OBSSettingsState::Draw() {
    constexpr float labelW = 200.0f;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.60f,1.0f));
    ImGui::TextUnformatted("CONNECTION");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Text("Host");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(240.0f);
    if (ImGui::InputText("##obs_host", m_host, sizeof(m_host))) CheckDirty();

    ImGui::Text("Port");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("##obs_port", &m_port, 0, 0)) {
        m_port = std::max(1, std::min(65535, m_port));
        CheckDirty();
    }
}

// ─── Save ──────────────────────────────────────────────────────────────────────
void OBSSettingsState::SaveChanges() const {
    auto* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    cfg->obsHost = m_host;
    cfg->obsPort = m_port;

    if (!cfg->Save()) {
        std::cerr << "[Settings/RecordingSettingsState] Config save failed" << std::endl;
    } else {
        std::cout << "[Settings/RecordingSettingsState] Config updated successfully" << std::endl;
    }
}