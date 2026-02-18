#include "gui/screens/main/TopBar.h"

#include "core/CoreServices.h"
#include "core/recording/RecordingManager.h"
#include "gui/screens/main/MainScreen.h"

#include <filesystem>
#include <sys/statvfs.h>

#include "imgui.h"

void TopBar::Draw(MainScreen* parent, const RecordingManager* recordingManager) const {
    constexpr float barHeight = 40.0f;
    ImGui::BeginChild("TopBar", ImVec2(0, barHeight), false, ImGuiWindowFlags_NoScrollbar);

    auto& services = CoreServices::Instance();
    const auto* config = services.GetConfig();
    const auto& videos = parent->GetCurrentVideos();
    const StorageInfo storageInfo = CalculateStorageInfo(config->libraryPath, videos.size());

    constexpr float contentHeight = 40.0f;
    ImGui::SetCursorPosY((barHeight - contentHeight) * 0.5f);

    ImGui::BeginGroup();
    DrawStorageInfo(storageInfo);
    ImGui::EndGroup();

    constexpr float recordBtnWidth = 180.0f;
    constexpr float settingsBtnWidth = 40.0f;
    constexpr float innerPadding = 15.0f;
    constexpr float edgePadding = 20.0f;
    constexpr float rightBlockWidth = recordBtnWidth + innerPadding + settingsBtnWidth + edgePadding;

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - rightBlockWidth);
    ImGui::SetCursorPosY((barHeight - 40.0f) * 0.5f);

    DrawRecordingButton(recordingManager);

    ImGui::SameLine(0, 15);
    if (ImGui::Button("S", ImVec2(settingsBtnWidth, 40))) {
        if (m_onSettingsClicked) m_onSettingsClicked();
    }

    ImGui::EndChild();
}

void TopBar::DrawRecordingButton(const RecordingManager* recordingManager) const {
    const bool isRecording = recordingManager && recordingManager->IsRecording();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    constexpr auto size = ImVec2(180, 40);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));

    if (ImGui::Button("##RecordBtn", size)) {
        if (m_onRecordClicked) m_onRecordClicked();
    }

    ImGui::PopStyleColor(3);

    constexpr float radius = 5.0f;
    const auto dotPos = ImVec2(pos.x + 20, pos.y + size.y * 0.5f);
    const ImVec4 dotColor = isRecording ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f) : ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

    if (isRecording) {
        const float time = static_cast<float>(ImGui::GetTime());
        for (int i = 0; i < 2; i++) {
            const float wave = fmodf(time + (i * 0.8f), 1.5f) / 1.5f;
            const float waveRadius = radius + (wave * 10.0f);
            const float alpha = 1.0f - wave;

            drawList->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
            drawList->AddCircle(dotPos, waveRadius, ImColor(1.0f, 0.2f, 0.2f, alpha * 0.6f), 24, 1.5f);
            drawList->PopClipRect();
        }
    }

    drawList->AddCircleFilled(dotPos, radius, ImColor(dotColor), 24);

    const char* label = isRecording ? "STOP RECORDING" : "START RECORD";
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const auto textPos = ImVec2(pos.x + 35, pos.y + (size.y - textSize.y) * 0.5f);
    drawList->AddText(textPos, ImColor(1.0f, 1.0f, 1.0f, 1.0f), label);

    if (recordingManager) {
        const char* mode = recordingManager->GetMode() == RecordingMode::OBS ? "OBS" : "NAT";
        const ImVec2 modeSize = ImGui::CalcTextSize(mode);
        drawList->AddText(ImVec2(pos.x + size.x - modeSize.x - 10, textPos.y), ImColor(0.4f, 0.4f, 0.4f, 1.0f), mode);
    }
}

void TopBar::DrawStorageInfo(const StorageInfo& info) {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12);

    ImGui::Text("%zu VIDEOS", info.totalVideos);
    ImGui::SameLine(0, 20);
    ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.2f, 1.0f), "|");
    ImGui::SameLine(0, 20);

    ImGui::Text("%.1f GB USED", info.usedSpaceGB);
    ImGui::SameLine(0, 20);
    ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.2f, 1.0f), "|");
    ImGui::SameLine(0, 20);

    const ImVec4 statusColor = (info.usedSpaceGB / info.totalSpaceGB) > 0.9f ? ImVec4(1, 0.3f, 0.3f, 1) : ImVec4(0.4f, 0.8f, 0.4f, 1);
    ImGui::TextColored(statusColor, "%.1f GB FREE", info.freeSpaceGB);
}

StorageInfo TopBar::CalculateStorageInfo(const std::string& libraryPath, size_t videoCount) {
    StorageInfo info;
    info.totalVideos = videoCount;
    if (libraryPath.empty() || !std::filesystem::exists(libraryPath)) return info;

    struct statvfs stat;
    if (statvfs(libraryPath.c_str(), &stat) == 0) {
        constexpr double gb = 1024.0 * 1024.0 * 1024.0;
        info.totalSpaceGB = (stat.f_blocks * static_cast<double>(stat.f_frsize)) / gb;
        info.freeSpaceGB = (stat.f_bfree * static_cast<double>(stat.f_frsize)) / gb;
        info.usedSpaceGB = info.totalSpaceGB - info.freeSpaceGB;
    }
    return info;
}