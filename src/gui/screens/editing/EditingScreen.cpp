#include "gui/screens/editing/EditingScreen.h"

#include "gui/screens/editing/states/VideoEditState.h"
#include "gui/screens/editing/ExportWidget.h"
#include "gui/core/MainWindow.h"

#include "imgui.h"

EditingScreen::EditingScreen(MainWindow *manager)
    : BaseScreen(manager){

    // Create states
    m_videoEditState = std::make_unique<VideoEditState>();
    m_exportWidget = std::make_unique<ExportWidget>();

}

void EditingScreen::Close() const {
    GetManager()->SetApplicationState(ApplicationState::MAIN);
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
    m_videoEditState->Draw(this);
    ImGui::End();

    if (m_showExportWidget) {
        m_exportWidget->Draw(this);
    }
}


const char* EditingScreen::GetCurrentWindowName() const {
    switch (m_currentState) {
        case EditingScreenState::VIDEO_EDIT:        return "VideoEdit";
        default:                                    return "EditingScreen";
    }
}
