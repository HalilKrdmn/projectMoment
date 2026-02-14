#include "gui/screens/main/states/LoadingState.h"
#include "gui/screens/main/MainScreen.h"
#include <sstream>
#include <iomanip>

void LoadingState::Draw(MainScreen* parent) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::SetCursorPos(ImVec2(avail.x * 0.5f - 100, avail.y * 0.3f));
    ImGui::PushFont(nullptr);
    ImGui::Text("Loading Videos...");
    ImGui::PopFont();

    ImGui::SetCursorPosX(avail.x * 0.3f);
    ImGui::ProgressBar(m_progress, ImVec2(avail.x * 0.4f, 30));
    ImGui::SameLine();
    ImGui::Text("%.0f%%", m_progress * 100.0f);

    ImGui::Dummy(ImVec2(0, 20));

    ImGui::SetCursorPos(ImVec2(avail.x * 0.2f, avail.y * 0.45f));
    ImGui::BeginChild("LoadingLogs", ImVec2(avail.x * 0.6f, avail.y * 0.4f),
                      true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    {
        std::lock_guard lock(m_logMutex);
        
        for (const auto& [message, progress, timestamp] : m_logs) {
            auto time_t = std::chrono::system_clock::to_time_t(timestamp);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");

            const char* icon = "⏳";
            ImVec4 color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            
            if (progress >= 1.0f) {
                icon = "✓";
                color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
            } else if (progress < 0.0f) {
                icon = "✗";
                color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            }

            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", ss.str().c_str());
            ImGui::SameLine();
            ImGui::TextColored(color, "%s", icon);
            ImGui::SameLine();
            ImGui::Text("%s", message.c_str());

            if (progress >= 0.0f && progress < 1.0f) {
                ImGui::ProgressBar(progress, ImVec2(-1, 0));
            }
        }
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

void LoadingState::AddLog(const std::string& message, float progress) {
    std::lock_guard lock(m_logMutex);
    
    LoadingLog log;
    log.message = message;
    log.progress = progress;
    log.timestamp = std::chrono::system_clock::now();
    
    m_logs.push_back(log);

    if (m_logs.size() > 100) {
        m_logs.erase(m_logs.begin());
    }
}

void LoadingState::Clear() {
    std::lock_guard lock(m_logMutex);
    m_logs.clear();
    m_progress = 0.0f;
}