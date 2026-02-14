#include "gui/screens/editing/EditingScreen.h"

#include "gui/screens/editing/states/VideoEditState.h"
#include "gui/screens/editing/states/ExportState.h"

#include "imgui.h"

EditingScreen::EditingScreen(MainWindow *manager)
    : BaseScreen(manager){

    // Create states
    m_videoEditState = std::make_unique<VideoEditState>();
    m_exportState = std::make_unique<ExportState>();

}

void EditingScreen::Draw() {
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoBackground;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin(GetCurrentWindowName(), nullptr, flags);

    switch (m_currentState) {
        case EditingScreenState::VIDEO_EDIT:
            m_videoEditState->Draw(this);
            break;
        case EditingScreenState::EXPORT:
            m_exportState->Draw(this);
            break;
    }

    ImGui::End();
}

const char* EditingScreen::GetCurrentWindowName() const {
    switch (m_currentState) {
        case EditingScreenState::VIDEO_EDIT:        return "VideoEdit";
        case EditingScreenState::EXPORT:            return "Export";
        default:                                    return "EditingScreen";
    }
}