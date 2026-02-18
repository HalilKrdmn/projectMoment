#include "gui/screens/main/states/VideoListState.h"
#include "gui/screens/main/MainScreen.h"

#include "gui/utils/ThumbnailLoader.h"
#include "gui/utils/FormatUtils.h"
#include "data/VideoInfo.h"

#include <algorithm>
#include <cmath>

#include "core/recording/RecordingManager.h"

MainWindow* VideoListState::GetMainWindow(const MainScreen* parent) {
    return parent->GetManager();
}

void VideoListState::Draw(MainScreen* parent) {
    if (!m_thumbnailsLoaded) {
        LoadThumbnails(parent);
        m_thumbnailsLoaded = true;
    }

    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));
    DrawVideoGrid(parent);
}

void VideoListState::DrawVideoGrid(MainScreen* parent) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const auto& videos = parent->GetCurrentVideos();

    constexpr float thumbnailSize = 200.0f;
    constexpr float padding = 15.0f;
    constexpr float tHeight = thumbnailSize * 9.0f / 16.0f;
    constexpr float totalItemWidth = thumbnailSize;

    const int columns = std::max(1, static_cast<int>(avail.x / (totalItemWidth + padding)));

    ImGui::BeginChild("VideoGrid", ImVec2(0, 0), false);

    int currentPos = 0;

    for (size_t i = 0; i < videos.size(); i++) {
        ImGui::PushID(static_cast<int>(i));
        auto& video = videos[i];

        ImGui::BeginGroup();

        // Thumbnail
        if (video.thumbnailId) {
            ImGui::Image(video.thumbnailId, ImVec2(thumbnailSize, tHeight));
        } else {
            ImVec2 pMin = ImGui::GetCursorScreenPos();
            auto pMax = ImVec2(pMin.x + thumbnailSize, pMin.y + tHeight);
            ImGui::GetWindowDrawList()->AddRectFilled(pMin, pMax, IM_COL32(50, 50, 50, 255), 8.0f);

            ImGui::SetCursorScreenPos(ImVec2(pMin.x + 10, pMin.y + tHeight * 0.4f));
            ImGui::TextDisabled("  Generating\n Thumbnail...");

            ImGui::Dummy(ImVec2(thumbnailSize, tHeight));
        }

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + thumbnailSize);
        ImGui::TextWrapped("%s", video.name.c_str());

        std::string dateStr = FormatUtils::FormatDate(video.recordingTimeMs);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", dateStr.c_str());

        std::string durationStr = FormatUtils::FormatDuration(video.durationSec);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "| %s", durationStr.c_str());

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                          "%dx%d", video.resolutionWidth, video.resolutionHeight);

        ImGui::PopTextWrapPos();
        ImGui::EndGroup();

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            if (MainWindow* mainWindow = GetMainWindow(parent)) {
                mainWindow->SwitchToEditingScreen(video);
            }
        }

        // Context menu
        char popupID[64];
        snprintf(popupID, sizeof(popupID), "VideoContext##%zu", i);

        if (ImGui::BeginPopupContextItem(popupID)) {
            if (ImGui::MenuItem("Edit")) {}
            if (ImGui::MenuItem("Properties")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {}
            ImGui::EndPopup();
        }

        // Grid layout
        currentPos++;
        if (currentPos % columns != 0 && (i < videos.size() - 1)) {
            ImGui::SameLine(0, padding);
        } else {
            ImGui::Dummy(ImVec2(0, padding));
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void VideoListState::LoadThumbnails(MainScreen* parent) {
    ThumbnailLoader::LoadThumbnails(parent->GetCurrentVideos());
}