#include "gui/screens/main/states/LoadingState.h"
#include "gui/Theme.h"
#include "gui/screens/main/MainScreen.h"
#include <sstream>
#include <iomanip>

void LoadingState::Draw(MainScreen*) {
    const ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::SetCursorPos(ImVec2(avail.x * 0.5f - 80, avail.y * 0.28f));
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_PRIMARY);
    ImGui::TextUnformatted("Loading Videos...");
    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(avail.x * 0.3f);
    ImGui::ProgressBar(m_progress, ImVec2(avail.x * 0.4f, 28));
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::Text("%.0f%%", m_progress * 100.0f);
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 16));

    ImGui::SetCursorPosX(avail.x * 0.2f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BG_DARK);
    ImGui::BeginChild("LoadingLogs",
        ImVec2(avail.x * 0.6f, avail.y * 0.4f), true,
        ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::PopStyleColor();

    {
        std::lock_guard lock(m_logMutex);
        for (const auto& [message, progress, timestamp] : m_logs) {
            auto tt = std::chrono::system_clock::to_time_t(timestamp);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&tt), "%H:%M:%S");

            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
            ImGui::TextUnformatted(ss.str().c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine();

            if (progress >= 1.0f) {
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::SUCCESS);
                ImGui::TextUnformatted("✓");
            } else if (progress < 0.0f) {
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::DANGER);
                ImGui::TextUnformatted("✗");
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::TextUnformatted("⏳");
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_PRIMARY);
            ImGui::TextUnformatted(message.c_str());
            ImGui::PopStyleColor();

            if (progress >= 0.0f && progress < 1.0f)
                ImGui::ProgressBar(progress, ImVec2(-1, 0));
        }
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

void LoadingState::AddLog(const std::string& message, float progress) {
    std::lock_guard lock(m_logMutex);
    LoadingLog log;
    log.message   = message;
    log.progress  = progress;
    log.timestamp = std::chrono::system_clock::now();
    m_logs.push_back(log);
    if (m_logs.size() > 100)
        m_logs.erase(m_logs.begin());
}

void LoadingState::Clear() {
    std::lock_guard lock(m_logMutex);
    m_logs.clear();
    m_progress = 0.0f;
}