#include "gui/screens/main/states/WelcomeState.h"
#include "gui/screens/main/MainScreen.h"

#include <algorithm>

void WelcomeState::Draw(MainScreen* parent) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::GetStyle();

    // Update cache
    if (ImFont* currentFont = ImGui::GetFont();
        !m_cache.initialized ||
        m_cache.lastFont != currentFont ||
        m_cache.lastAvailSize.x != avail.x ||
        m_cache.lastAvailSize.y != avail.y) {

        UpdateCache();
        m_cache.lastFont = currentFont;
        m_cache.lastAvailSize = avail;
        m_cache.initialized = true;
    }

    ImGui::SetCursorPos(m_cache.startPos);

    const float titleOffsetX = (m_cache.contentWidth - m_cache.primarySize.x) * 0.5f;
    ImGui::SetCursorPosX(m_cache.startPos.x + titleOffsetX);
    ImGui::PushFont(nullptr);
    ImGui::Text("%s", m_cache.primaryText.c_str());
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0, 10.0f));
    const float subtitleOffsetX = (m_cache.contentWidth - m_cache.secondarySize.x) * 0.5f;
    ImGui::SetCursorPosX(m_cache.startPos.x + subtitleOffsetX);
    ImGui::Text("%s", m_cache.secondaryText.c_str());

    ImGui::Dummy(ImVec2(0, 10.0f));
    const float instructionOffsetX = (m_cache.contentWidth - m_cache.tertiarySize.x) * 0.5f;
    ImGui::SetCursorPosX(m_cache.startPos.x + instructionOffsetX);
    ImGui::Text("%s", m_cache.tertiaryText.c_str());

    ImGui::Dummy(ImVec2(0, 20.0f));
    const float buttonOffsetX = (m_cache.contentWidth - m_cache.buttonSize.x) * 0.5f;
    ImGui::SetCursorPosX(m_cache.startPos.x + buttonOffsetX);

    if (ImGui::Button(m_cache.buttonText.c_str())) {
        parent->GetFolderBrowser().Open();
    }

    if (parent->GetFolderBrowser().HasSelected()) {
        parent->DetermineInitialState();
    }
}

void WelcomeState::UpdateCache() {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImGuiStyle& style = ImGui::GetStyle();

    ImGui::PushFont(nullptr);
    m_cache.primarySize = ImGui::CalcTextSize(m_cache.primaryText.c_str());
    ImGui::PopFont();

    m_cache.secondarySize = ImGui::CalcTextSize(m_cache.secondaryText.c_str());
    m_cache.tertiarySize = ImGui::CalcTextSize(m_cache.tertiaryText.c_str());

    const ImVec2 buttonTextSize = ImGui::CalcTextSize(m_cache.buttonText.c_str());
    m_cache.buttonSize = ImVec2(
        buttonTextSize.x + style.FramePadding.x * 2.0f,
        ImGui::GetFrameHeight()
    );

    m_cache.contentWidth = std::max({
        m_cache.primarySize.x,
        m_cache.secondarySize.x,
        m_cache.tertiarySize.x,
        m_cache.buttonSize.x
    });

    constexpr float spacing = 10.0f;
    constexpr float dummySize = 20.0f;
    m_cache.contentHeight =
        m_cache.primarySize.y + spacing +
        m_cache.secondarySize.y + spacing +
        m_cache.tertiarySize.y + dummySize +
        m_cache.buttonSize.y;

    m_cache.startPos = ImVec2(
        (avail.x - m_cache.contentWidth) * 0.5f,
        (avail.y - m_cache.contentHeight) * 0.5f
    );
}