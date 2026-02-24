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

    {
        ImDrawList*  bg     = ImGui::GetBackgroundDrawList();
        const ImVec2 origin = vp->WorkPos;
        const float  totalW = vp->WorkSize.x;
        const float  totalH = vp->WorkSize.y;

        bg->AddRectFilled(
            origin,
            { origin.x + SIDEBAR_WIDTH, origin.y + totalH },
            ImGui::GetColorU32(Theme::BG_DARK)
        );
        bg->AddRectFilled(
            { origin.x + SIDEBAR_WIDTH, origin.y },
            { origin.x + totalW,        origin.y + totalH },
            ImGui::GetColorU32(Theme::BG_CONTENT)
        );

        // Top bar strip: full width at BG_DARK (covers both sidebar and content)
        bg->AddRectFilled(
            origin,
            { origin.x + totalW, origin.y + Theme::TOPBAR_H },
            ImGui::GetColorU32(Theme::BG_DARK)
        );
    }

    // Divider line (ForegroundDrawList)
    {
        ImDrawList*  fg     = ImGui::GetForegroundDrawList();
        const ImVec2 origin = vp->WorkPos;
        const float  lineY  = origin.y + Theme::TOPBAR_H;
        fg->AddLine(
            { origin.x,                  lineY },
            { origin.x + vp->WorkSize.x, lineY },
            Theme::SEPARATOR_LINE, 1.0f
        );

        // Vertical separator between sidebar and content
        fg->AddLine(
            { origin.x + SIDEBAR_WIDTH, origin.y },
            { origin.x + SIDEBAR_WIDTH, origin.y + vp->WorkSize.y },
            Theme::SEPARATOR_LINE, 1.0f
        );
    }

    // Zero out parent padding — child windows must start at the exact edge
    // to match the BackgroundDrawList rects (which use absolute screen coords).
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Settings", nullptr, flags);
    ImGui::PopStyleVar();

    const float dt = ImGui::GetIO().DeltaTime;
    m_selectorY += (m_selectorTargetY - m_selectorY) * ANIM_SPEED * dt;

    DrawSidebar();
    DrawContent();

    ImGui::End();
}

// ─── Sidebar ─────────────────────────────────────────────────────────────────
void SettingsScreen::DrawSidebar() {
    const ImGuiViewport* vp     = ImGui::GetMainViewport();
    const float          totalH = vp->WorkSize.y;

    m_itemsStartY = Theme::TOPBAR_H + 10.0f;

    constexpr float SAVE_BAR_H = 52.0f;

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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BG_DARK);
    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("##settings_sidebar", ImVec2(SIDEBAR_WIDTH, totalH), false,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();

    // ── "SETTINGS" label ──────────────────────────────────────────────────────
    ImGui::SetCursorPosY((Theme::TOPBAR_H - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::SetCursorPosX(18.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("SETTINGS");
    ImGui::PopStyleColor();

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
            active ? ImGui::GetColorU32(Theme::TEXT_PRIMARY)
                   : ImGui::GetColorU32(Theme::TEXT_MUTED),
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
    ImGui::PushStyleColor(ImGuiCol_Button,        dirty ? Theme::ACCENT : Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT_HOVER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::ACCENT_ACTIVE);
    if (ImGui::Button("Save Changes", ImVec2(SIDEBAR_WIDTH - 16.0f, 34.0f))) SaveCurrent();
    ImGui::PopStyleColor(3);
    ImGui::EndDisabled();

    ImGui::Dummy(ImVec2(SIDEBAR_WIDTH - 16.0f, 0.0f));

    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg
}

// ─── Content ─────────────────────────────────────────────────────────────────
void SettingsScreen::DrawContent() const {
    const ImGuiViewport* vp    = ImGui::GetMainViewport();
    const float          contW = vp->WorkSize.x - SIDEBAR_WIDTH;
    const float          contH = vp->WorkSize.y;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BG_CONTENT);
    ImGui::SetCursorPos(ImVec2(SIDEBAR_WIDTH, 0.0f));
    ImGui::BeginChild("##settings_content", ImVec2(contW, contH), false);
    ImGui::PopStyleVar();

    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        dl->AddRectFilled(
            wp,
            { wp.x + contW, wp.y + Theme::TOPBAR_H },
            ImGui::GetColorU32(Theme::BG_DARK)
        );
    }

    // ── Back button ───────────────────────────────────────────────────────────
    const float btnX = contW - Theme::TOPBAR_BTN_PAD - Theme::TOPBAR_BTN_W - 10.0f;
    const float btnY = (Theme::TOPBAR_H - Theme::TOPBAR_BTN_H) * 0.5f;
    ImGui::SetCursorPos(ImVec2(btnX, btnY));
    ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::DANGER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::DANGER);
    if (ImGui::Button("X", ImVec2(Theme::TOPBAR_BTN_W, Theme::TOPBAR_BTN_H)))
        GetManager()->SetApplicationState(ApplicationState::MAIN);
    ImGui::PopStyleColor(3);

    // ── Section content ───────────────────────────────────────────────────────
    constexpr float LEFT_PAD  = 24.0f;
    constexpr float RIGHT_PAD = 40.0f;
    const float childW = contW - LEFT_PAD - RIGHT_PAD;

    ImGui::SetCursorPos(ImVec2(LEFT_PAD, Theme::TOPBAR_H));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BG_CONTENT);
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
    ImGui::PopStyleColor(); // ChildBg
}