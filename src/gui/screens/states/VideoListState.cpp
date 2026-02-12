#include "gui/screens/states/VideoListState.h"

#include "gui/screens/MainScreen.h"
#include "gui/utils/ThumbnailLoader.h"
#include "gui/utils/FormatUtils.h"
#include "data/VideoInfo.h"

#include <algorithm>


void VideoListState::Draw(MainScreen* parent) {
    if (!m_thumbnailsLoaded) {
        LoadThumbnails(parent);
        m_thumbnailsLoaded = true;
    }
    
    DrawHeader(parent);
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));
    DrawVideoGrid(parent);
}

void VideoListState::DrawHeader(MainScreen* parent) {
    const auto& videos = parent->GetCurrentVideos();
    
    ImGui::Text("Videos: %zu", videos.size());
    ImGui::SameLine();
    
    if (ImGui::Button("Change Folder")) {
        parent->GetFolderBrowser().Open();
    }
    
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                       "| %s", parent->GetCurrentFolder().string().c_str());
}

void VideoListState::DrawVideoGrid(MainScreen* parent) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const auto& videos = parent->GetCurrentVideos();

    float thumbnailSize = 200.0f;
    float padding = 15.0f;
    float itemWidth = thumbnailSize + padding * 2;
    int columns = std::max(1, (int)(avail.x / itemWidth));
    
    ImGui::BeginChild("VideoGrid", ImVec2(0, 0), false);
    
    for (size_t i = 0; i < videos.size(); i++) {
        ImGui::PushID(static_cast<int>(i));
        
        auto& video = videos[i];
        
        ImGui::BeginGroup();
        
        // Thumbnail
        if (video.thumbnailId) {
            ImGui::Image(video.thumbnailId, ImVec2(thumbnailSize, thumbnailSize * 9.0f / 16.0f));
        } else {
            ImGui::Button("No Thumbnail", ImVec2(thumbnailSize, thumbnailSize * 9.0f / 16.0f));
        }
        
        // Video bilgileri
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + thumbnailSize);
        
        ImGui::TextWrapped("%s", video.name.c_str());
        
        std::string dateStr = FormatUtils::FormatDate(video.recordingTimeMs);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "📅 %s", dateStr.c_str());

        std::string durationStr = FormatUtils::FormatDuration(video.durationSec);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "| ⏱️ %s", durationStr.c_str());
        
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                          "📐 %dx%d", video.resolutionWidth, video.resolutionHeight);
        
        ImGui::PopTextWrapPos();
        ImGui::EndGroup();
        
        // Click handler
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            // TODO: Video seçildi
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
        if ((i + 1) % columns != 0 && i < videos.size() - 1) {
            ImGui::SameLine();
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