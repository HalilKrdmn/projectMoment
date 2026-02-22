#include "gui/screens/settings/SettingsScreen.h"

#include "gui/Theme.h"
#include "gui/core/MainWindow.h"
#include "gui/screens/settings/states/GeneralSettingsState.h"
#include "gui/screens/settings/states/RecordingSettingsState.h"
#include "gui/screens/settings/states/OBSSettingsState.h"
#include "gui/screens/settings/states/NativeRecordingSettingsState.h"

// ─────────────────────────────────────────────────────────────────────────────
struct SidebarItem {
    const char*     label;
    SettingsSection section;
};

static constexpr SidebarItem k_items[] = {
    { "General",          SettingsSection::GENERAL          },
    { "Recording",        SettingsSection::RECORDING        },
    { "OBS",              SettingsSection::OBS              },
    { "Native Recording", SettingsSection::NATIVE_RECORDING },
};
static constexpr int k_itemCount = std::size(k_items);
// ─────────────────────────────────────────────────────────────────────────────

SettingsScreen::SettingsScreen(MainWindow* manager)
    : BaseScreen(manager)
{
    m_generalState   = std::make_unique<GeneralSettingsState>();
    m_recordingState = std::make_unique<RecordingSettingsState>();
    m_obsState       = std::make_unique<OBSSettingsState>();
    m_nativeState    = std::make_unique<NativeRecordingSettingsState>();
}

void SettingsScreen::SelectSection(const SettingsSection section) {
    m_currentSection = section;

    for (int i = 0; i < k_itemCount; ++i) {
        if (k_items[i].section == section) {
            const float target = m_itemsStartY + i * ITEM_HEIGHT;
            m_selectorTargetY  = target;
            break;
        }
    }
}

// ─── Helpers ─────────────────────────────────────────────────────────────────
bool SettingsScreen::IsCurrentDirty() const {
    switch (m_currentSection) {
        case SettingsSection::GENERAL:          return m_generalState->IsDirty();
        case SettingsSection::RECORDING:        return m_recordingState->IsDirty();
        case SettingsSection::OBS:              return m_obsState->IsDirty();
        case SettingsSection::NATIVE_RECORDING: return m_nativeState->IsDirty();
    }
    return false;
}

void SettingsScreen::SaveCurrent() const {
    switch (m_currentSection) {
        case SettingsSection::GENERAL:          m_generalState->Save();   break;
        case SettingsSection::RECORDING:        m_recordingState->Save(); break;
        case SettingsSection::OBS:              m_obsState->Save();       break;
        case SettingsSection::NATIVE_RECORDING: m_nativeState->Save();    break;
    }
}

// ─── Main Draw ───────────────────────────────────────────────────────────────
void SettingsScreen::Draw() {
    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize     |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGui::Begin("Settings", nullptr, flags);

    const float dt = ImGui::GetIO().DeltaTime;
    m_selectorY += (m_selectorTargetY - m_selectorY) * ANIM_SPEED * dt;

    DrawSidebar();
    ImGui::SameLine(0.0f, 0.0f);
    DrawContent();
    DrawDividerLine();

    ImGui::End();
}

// ─── Sidebar ─────────────────────────────────────────────────────────────────
void SettingsScreen::DrawSidebar() {
    const ImGuiViewport* vp     = ImGui::GetMainViewport();
    const float          totalH = vp->WorkSize.y;

    // Items start just below the header
    constexpr float HEADER_OFFSET = 62.0f;
    constexpr float SAVE_BAR_H    = 52.0f;
    m_itemsStartY = HEADER_OFFSET;

    // First frame: snap selector
    if (!m_selectorReady) {
        for (int i = 0; i < k_itemCount; ++i) {
            if (k_items[i].section == m_currentSection) {
                m_selectorY = m_selectorTargetY = m_itemsStartY + i * ITEM_HEIGHT;
                break;
            }
        }
        m_selectorReady = true;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BG_DARK);
    ImGui::BeginChild("##settings_sidebar", ImVec2(SIDEBAR_WIDTH, totalH), false,
                      ImGuiWindowFlags_NoScrollbar);

    // ── App title ────────────────────────────────────────────────────────────
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 18.0f);
    ImGui::SetCursorPosX(18.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("SETTINGS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // ── Animated selection highlight ─────────────────────────────────────────
    {
        ImDrawList*  dl     = ImGui::GetWindowDrawList();
        const ImVec2 winPos = ImGui::GetWindowPos();
        const float  rx     = winPos.x + 8.0f;
        const float  ry     = winPos.y + m_selectorY;
        dl->AddRectFilled(
            ImVec2(rx, ry),
            ImVec2(rx + SIDEBAR_WIDTH - 16.0f, ry + ITEM_HEIGHT - 4.0f),
            Theme::SELECTOR_BG,
            6.0f
        );
    }

    // ── Items ────────────────────────────────────────────────────────────────
    ImGui::SetCursorPosY(m_itemsStartY);

    for (int i = 0; i < k_itemCount; ++i) {
        const auto& [label, section] = k_items[i];
        const bool  active           = (m_currentSection == section);

        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0,0,0,0));

        ImGui::SetCursorPosX(8.0f);
        const float curY    = ImGui::GetCursorPosY();
        const bool  clicked = ImGui::InvisibleButton(
            label,
            ImVec2(SIDEBAR_WIDTH - 16.0f, ITEM_HEIGHT - 4.0f)
        );

        ImDrawList*  dl   = ImGui::GetWindowDrawList();
        const ImVec2 vec2 = ImGui::GetWindowPos();
        dl->AddText(
            { vec2.x + 18.0f, vec2.y + curY + 12.0f },
            active ? ImGui::GetColorU32(Theme::TEXT_PRIMARY) : ImGui::GetColorU32(Theme::TEXT_MUTED),
            label
        );

        if (clicked) SelectSection(section);

        ImGui::PopStyleColor(4);
        ImGui::SetCursorPosY(curY + ITEM_HEIGHT);
    }

    // ── Save Changes ──────────────────────────────────────────────────────────
    const bool dirty = IsCurrentDirty();

    ImGui::SetCursorPosY(totalH - SAVE_BAR_H + (SAVE_BAR_H - 34.0f) * 0.5f - 20.0f);
    ImGui::SetCursorPosX(8.0f);

    ImGui::BeginDisabled(!dirty);
    ImGui::PushStyleColor(ImGuiCol_Button,dirty
        ? Theme::ACCENT
        : Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT_HOVER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::ACCENT_ACTIVE);
    if (ImGui::Button("Save Changes", ImVec2(SIDEBAR_WIDTH - 16.0f, 34.0f))) SaveCurrent();
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    ImGui::Dummy(ImVec2(SIDEBAR_WIDTH - 16.0f, 0.0f));

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

// ─── Content ─────────────────────────────────────────────────────────────────
void SettingsScreen::DrawContent() const {
    const ImGuiViewport* vp    = ImGui::GetMainViewport();
    const float          contW = vp->WorkSize.x - SIDEBAR_WIDTH;
    const float          contH = vp->WorkSize.y;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BG_CONTENT);
    ImGui::BeginChild("##settings_content", ImVec2(contW, contH), false);
    ImGui::PopStyleVar();

    // ── Header: title left | X right ─────────────────────────────────────────
    constexpr float HEADER_H = 52.0f;
    constexpr float BTN_W    = 32.0f;
    constexpr float BTN_H    = 32.0f;
    constexpr float BTN_PAD  = 10.0f;

    ImGui::SetCursorPos(ImVec2(24.0f, (HEADER_H - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_PRIMARY);
    auto title = "General";
    switch (m_currentSection) {
        case SettingsSection::GENERAL:          title = "General";          break;
        case SettingsSection::RECORDING:        title = "Recording";        break;
        case SettingsSection::OBS:              title = "OBS";              break;
        case SettingsSection::NATIVE_RECORDING: title = "Native Recording"; break;
    }
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();

    // X — Exit
    ImGui::SetCursorPos(ImVec2(contW - BTN_PAD - BTN_W - 20.0f, (HEADER_H - BTN_H) * 0.5f - 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::DANGER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::DANGER);
    if (ImGui::Button("X", ImVec2(BTN_W, BTN_H)))
        GetManager()->SetApplicationState(ApplicationState::MAIN);
    ImGui::PopStyleColor(3);

    // Dummy
    ImGui::SetCursorPosY(HEADER_H);
    ImGui::Dummy(ImVec2(contW - 24.0f, 0.0f));

    // Separator
    ImGui::SetCursorPosY(HEADER_H - 1.0f);
    ImGui::SetCursorPosX(24.0f);
    ImGui::SetCursorPosX(24.0f);
    ImGui::Spacing();

    // ── Section content ───────────────────────────────────────────────────────
    constexpr float LEFT_PAD  = 24.0f;
    constexpr float RIGHT_PAD = 40.0f;
    const float childW = contW - LEFT_PAD - RIGHT_PAD;

    ImGui::SetCursorPosX(LEFT_PAD);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0));
    ImGui::BeginChild("##section_inner", ImVec2(childW, 0.0f), false);
    switch (m_currentSection) {
        case SettingsSection::GENERAL:          m_generalState->Draw();   break;
        case SettingsSection::RECORDING:        m_recordingState->Draw(); break;
        case SettingsSection::OBS:              m_obsState->Draw();       break;
        case SettingsSection::NATIVE_RECORDING: m_nativeState->Draw();    break;
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void SettingsScreen::DrawDividerLine() {
    const ImGuiViewport* vp  = ImGui::GetMainViewport();
    ImDrawList*          dl  = ImGui::GetForegroundDrawList();
    const ImVec2         wp  = ImGui::GetWindowPos();

    const float lineY     = wp.y + 52.0f;
    const float junctionX = wp.x + SIDEBAR_WIDTH + 7.0f;

    constexpr float gapSide = 8.0f;
    constexpr float gapMid  = 4.0f;
    constexpr ImU32 col     = Theme::SEPARATOR_LINE;

    dl->AddLine(ImVec2(wp.x + gapSide, lineY),
                ImVec2(junctionX - gapMid, lineY), col, 1.0f);

    dl->AddLine(ImVec2(junctionX + gapMid, lineY),
                ImVec2(wp.x + vp->WorkSize.x - gapSide, lineY), col, 1.0f);
}