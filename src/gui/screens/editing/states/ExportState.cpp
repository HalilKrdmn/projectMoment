#include "gui/screens/editing/states/ExportState.h"
#include "gui/screens/editing/EditingScreen.h"
#include "gui/core/MainWindow.h"
#include "imgui.h"

void ExportState::Draw(EditingScreen* parent) {
    const VideoInfo& video = parent->GetManager()->GetSelectedVideo();
    
    ImGui::Text("Export: %s", video.name.c_str());
    ImGui::Separator();
    
    ImGui::Text("Quality:");
    ImGui::SliderInt("##quality", &m_quality, 1, 100, "%d%%");
    
    ImGui::Separator();
    
    if (ImGui::Button("📤 Export Now", ImVec2(200, 40))) {
    }
    ImGui::SameLine();
    if (ImGui::Button("◀ Back to Edit", ImVec2(200, 40))) {
        parent->ChangeState(EditingScreenState::VIDEO_EDIT);
    }
}