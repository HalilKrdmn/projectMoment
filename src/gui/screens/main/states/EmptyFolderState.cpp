#include "gui/screens/main/states/EmptyFolderState.h"
#include "gui/screens/main/MainScreen.h"

void EmptyFolderState::Draw(MainScreen* parent) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::SetCursorPos(ImVec2(avail.x * 0.5f - 100, avail.y * 0.4f));
    ImGui::PushFont(nullptr);
    ImGui::Text("Empty Folder");
    ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(avail.x * 0.5f - 180, avail.y * 0.5f));
    ImGui::Text("No video files found in the selected folder");

    ImGui::SetCursorPos(ImVec2(avail.x * 0.5f - 150, avail.y * 0.55f));
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                       "Folder: %s", parent->GetCurrentFolder().string().c_str());

    ImGui::SetCursorPos(ImVec2(avail.x * 0.5f - 120, avail.y * 0.65f));
    if (ImGui::Button("Select Different Folder", ImVec2(240, 30))) {
        parent->GetFolderBrowser().Open();
    }
}