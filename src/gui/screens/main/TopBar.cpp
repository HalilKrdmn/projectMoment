#include "gui/screens/main/TopBar.h"

#include "core/CoreServices.h"
#include "gui/screens/main/MainScreen.h"

#include <cmath>
#include <filesystem>
#include <sys/statvfs.h>

#include "imgui.h"

void TopBar::Draw(MainScreen* parent, RecordingManager* recordingManager) {
    constexpr float barHeight = 40.0f;
    ImGui::BeginChild("TopBar", ImVec2(0, barHeight), false, ImGuiWindowFlags_NoScrollbar);

    auto& services = CoreServices::Instance();
    const auto* config = services.GetConfig();
    const auto& videos = parent->GetCurrentVideos();
    const StorageInfo storageInfo = CalculateStorageInfo(config->libraryPath, videos.size());

    ImGui::SetCursorPosY((barHeight - 40.0f) * 0.5f);
    ImGui::BeginGroup();
    DrawStorageInfo(storageInfo);
    ImGui::EndGroup();

    constexpr float recBtnW      = 155.0f;
    constexpr float clipBtnW     = 120.0f;
    constexpr float settingsBtnW = 40.0f;
    constexpr float gap          = 8.0f;
    constexpr float edgePad      = 20.0f;
    const float rightW = recBtnW + gap + clipBtnW + gap + settingsBtnW + edgePad;

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - rightW
                    + ImGui::GetCursorPosX() - ImGui::GetScrollX());
    ImGui::SetCursorPosY((barHeight - 40.0f) * 0.5f);

    DrawRecordToggleButton(recordingManager);
    ImGui::SameLine(0, gap);
    DrawClipSaveButton();

    ImGui::SameLine(0, gap);
    ImGui::SetCursorPosY((barHeight - 40.0f) * 0.5f);
    if (ImGui::Button("S", ImVec2(settingsBtnW, 40.0f))) {
        if (m_onSettingsClicked) m_onSettingsClicked();
    }

    ImGui::EndChild();
}


void TopBar::DrawRecordToggleButton(RecordingManager* recordingManager) {
    auto* recMgr = CoreServices::Instance().GetRecordingManager();
    const bool isRecording = recMgr && recMgr->IsRecording();
    const ImVec2 pos  = ImGui::GetCursorScreenPos();
    constexpr ImVec2 size = { 155.0f, 40.0f };
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (isRecording) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.12f,0.12f,0.12f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f,0.18f,0.18f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.08f,0.08f,0.08f,1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.14f,0.14f,0.18f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f,0.20f,0.26f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f,0.10f,0.14f,1.0f));
    }

    if (ImGui::Button("##RecBtn", size)) {
        if (recMgr) {
            if (isRecording) recMgr->StopRecording();
            else             recMgr->StartRecording();
        }
    }
    ImGui::PopStyleColor(3);

    constexpr float radius = 5.0f;
    const ImVec2 dotPos = { pos.x + 18.0f, pos.y + size.y * 0.5f };

    if (isRecording) {
        const float t = static_cast<float>(ImGui::GetTime());
        for (int i = 0; i < 2; ++i) {
            const float wave  = fmodf(t + i * 0.8f, 1.5f) / 1.5f;
            const float wR    = radius + wave * 10.0f;
            const float alpha = 1.0f - wave;
            dl->PushClipRect(pos, { pos.x + size.x, pos.y + size.y }, true);
            dl->AddCircle(dotPos, wR, ImColor(1.0f, 0.2f, 0.2f, alpha * 0.6f), 24, 1.5f);
            dl->PopClipRect();
        }
        dl->AddCircleFilled(dotPos, radius, ImColor(1.0f, 0.2f, 0.2f, 1.0f), 24);
    } else {
        dl->AddCircleFilled(dotPos, radius, ImColor(0.35f, 0.35f, 0.40f, 1.0f), 24);
    }

    const char* label = isRecording ? "STOP" : "START";
    const ImVec2 ts   = ImGui::CalcTextSize(label);
    dl->AddText({ pos.x + 32.0f, pos.y + (size.y - ts.y) * 0.5f },
                ImColor(1.0f, 1.0f, 1.0f, 1.0f), label);
}

void TopBar::DrawClipSaveButton() {
    auto* recMgr = CoreServices::Instance().GetRecordingManager();

    const bool isRecording = recMgr && recMgr->IsRecording();
    const bool isSaving    = recMgr && recMgr->IsSavingClip();

    ImGui::BeginDisabled(!recMgr || isSaving);

    if (isSaving) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::Button("Processing...", ImVec2(120.0f, 40.0f));
        ImGui::PopStyleColor();
    } else {
        if (isRecording) {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.20f, 0.50f, 0.90f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.60f, 1.00f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.15f, 0.40f, 0.80f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.45f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.25f, 0.25f, 0.30f, 1.0f));
        }

        if (ImGui::Button("SAVE CLIP", ImVec2(120.0f, 40.0f))) {
            recMgr->SaveClip();
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::EndDisabled();
}


void TopBar::DrawStorageInfo(const StorageInfo& info) {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12.0f);
    ImGui::Text("%zu VIDEOS", info.totalVideos);
    ImGui::SameLine(0, 20);
    ImGui::TextColored(ImVec4(0.2f,0.2f,0.2f,1.0f), "|");
    ImGui::SameLine(0, 20);
    ImGui::Text("%.1f GB USED", info.usedSpaceGB);
    ImGui::SameLine(0, 20);
    ImGui::TextColored(ImVec4(0.2f,0.2f,0.2f,1.0f), "|");
    ImGui::SameLine(0, 20);
    const float safeTotal = std::max(info.totalSpaceGB, 0.001f);
    const ImVec4 col = (info.usedSpaceGB / safeTotal) > 0.9f
        ? ImVec4(1.0f,0.3f,0.3f,1.0f) : ImVec4(0.4f,0.8f,0.4f,1.0f);
    ImGui::TextColored(col, "%.1f GB FREE", info.freeSpaceGB);
}

StorageInfo TopBar::CalculateStorageInfo(const std::string& libraryPath, size_t videoCount) {
    StorageInfo info;
    info.totalVideos = videoCount;
    if (libraryPath.empty() || !std::filesystem::exists(libraryPath)) return info;
    struct statvfs stat;
    if (statvfs(libraryPath.c_str(), &stat) == 0) {
        constexpr double gb = 1024.0 * 1024.0 * 1024.0;
        info.totalSpaceGB = static_cast<float>((stat.f_blocks * static_cast<double>(stat.f_frsize)) / gb);
        info.freeSpaceGB  = static_cast<float>((stat.f_bfree  * static_cast<double>(stat.f_frsize)) / gb);
        info.usedSpaceGB  = info.totalSpaceGB - info.freeSpaceGB;
    }
    return info;
}