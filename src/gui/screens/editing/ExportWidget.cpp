#include "gui/screens/editing/ExportWidget.h"

#include "gui/screens/editing/EditingScreen.h"
#include "gui/core/MainWindow.h"

#include "imgui.h"

void ExportWidget::Draw(const EditingScreen* parent) {
    if (!parent) return;

    DrawDimBackground();

    const VideoInfo& video = parent->GetManager()->GetSelectedVideo();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    constexpr auto windowSize = ImVec2(450, 320);
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetNextWindowFocus();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(25, 25));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("##ExportOverlayWindow", nullptr, flags)) {
        ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "EXPORT VIDEO");
        ImGui::Separator();
        ImGui::Text("Target File: %s", video.name.c_str());

        if (ImGui::Button("CANCEL", ImVec2(200, 45))) {
            parent->m_showExportWidget = false;
        }

        if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            parent->m_showExportWidget = false;
        }

        ImGui::End();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void ExportWidget::DrawDimBackground() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

    if (ImGui::Begin("##DimBackground", nullptr, flags)) {
        ImGui::End();
    }

    ImGui::PopStyleColor();
}