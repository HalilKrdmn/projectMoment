#include "gui/screens/settings/states/RecordingSettingsState.h"

#include "core/CoreServices.h"

#include <cstring>

RecordingSettingsState::RecordingSettingsState() {
    const auto* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    m_modeIndex = (cfg->recordingMode == "native") ? 1 : 0;
    std::strncpy(m_hotkeyRecordToggle, cfg->hotkeyRecordToggle.c_str(), sizeof(m_hotkeyRecordToggle) - 1);
    std::strncpy(m_hotkeySaveClip,     cfg->hotkeySaveClip.c_str(),     sizeof(m_hotkeySaveClip)     - 1);
    std::strncpy(m_hotkeyToggleMic,    cfg->hotkeyToggleMic.c_str(),    sizeof(m_hotkeyToggleMic)    - 1);
    SyncOriginals();
    m_dirty = false;
}

// ─── Dirty tracking ───────────────────────────────────────────────────────
void RecordingSettingsState::SyncOriginals() {
    m_origModeIndex = m_modeIndex;
    std::strncpy(m_origHotkeyRecordToggle, m_hotkeyRecordToggle, sizeof(m_origHotkeyRecordToggle) - 1);
    std::strncpy(m_origHotkeySaveClip,     m_hotkeySaveClip,     sizeof(m_origHotkeySaveClip)     - 1);
    std::strncpy(m_origHotkeyToggleMic,    m_hotkeyToggleMic,    sizeof(m_origHotkeyToggleMic)    - 1);
}

void RecordingSettingsState::CheckDirty() {
    m_dirty = (m_modeIndex != m_origModeIndex)
           || (std::strcmp(m_hotkeyRecordToggle, m_origHotkeyRecordToggle) != 0)
           || (std::strcmp(m_hotkeySaveClip,     m_origHotkeySaveClip)     != 0)
           || (std::strcmp(m_hotkeyToggleMic,    m_origHotkeyToggleMic)    != 0);
}

// ─── Draw ──────────────────────────────────────────────────────────────────
void RecordingSettingsState::Draw() {
    constexpr float labelW = 200.0f;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.60f,1.0f));
    ImGui::TextUnformatted("RECORDING MODE");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Text("Mode");
    ImGui::SameLine(labelW);

    ImGui::BeginGroup();
    bool changed = false;
    if (ImGui::RadioButton("OBS",    m_modeIndex == 0)) { m_modeIndex = 0; changed = true; }
    ImGui::SameLine();
    if (ImGui::RadioButton("Native", m_modeIndex == 1)) { m_modeIndex = 1; changed = true; }
    ImGui::EndGroup();
    if (changed) CheckDirty();

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f,0.10f,0.13f,1.0f));
    ImGui::BeginChild("##mode_desc", ImVec2(480.0f, 54.0f), true);
    ImGui::Spacing();
    ImGui::SetCursorPosX(10.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f,0.70f,0.75f,1.0f));
    if (m_modeIndex == 0)
        ImGui::TextWrapped("OBS mode delegates all recording to an OBS WebSocket connection.");
    else
        ImGui::TextWrapped("Native mode uses built-in capture. Configure codec and bitrate in the Native Recording section.");
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.60f,1.0f));
    ImGui::TextUnformatted("HOTKEYS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Text("Start / Stop Recording");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputText("##hotkey_record", m_hotkeyRecordToggle, sizeof(m_hotkeyRecordToggle))) CheckDirty();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.55f,1.0f));
    ImGui::TextUnformatted("(e.g. F10)");
    ImGui::PopStyleColor();

    ImGui::Text("Save Clip");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputText("##hotkey_clip", m_hotkeySaveClip, sizeof(m_hotkeySaveClip))) CheckDirty();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.55f,1.0f));
    ImGui::TextUnformatted("(e.g. F11)");
    ImGui::PopStyleColor();

    ImGui::Text("Microphone On / Off");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::InputText("##hotkey_mic", m_hotkeyToggleMic, sizeof(m_hotkeyToggleMic))) CheckDirty();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.55f,1.0f));
    ImGui::TextUnformatted("(e.g. F12)");
    ImGui::PopStyleColor();
}

// ─── Save ──────────────────────────────────────────────────────────────────────
void RecordingSettingsState::SaveChanges() const {
    auto* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    cfg->recordingMode      = (m_modeIndex == 1) ? "native" : "obs";
    cfg->hotkeyRecordToggle = m_hotkeyRecordToggle;
    cfg->hotkeySaveClip     = m_hotkeySaveClip;
    cfg->hotkeyToggleMic    = m_hotkeyToggleMic;

    if (!cfg->Save()) {
        std::cerr << "[Settings/RecordingSettingsState] Config save failed" << std::endl;
    } else {
        std::cout << "[Settings/RecordingSettingsState] Config updated successfully" << std::endl;
    }
}