#include "gui/screens/editing/ExportWidget.h"

#include "gui/screens/editing/EditingScreen.h"
#include "gui/core/MainWindow.h"
#include "gui/screens/editing/states/VideoEditState.h"

#include <cstring>
#include <thread>

#include "imgui.h"

ExportWidget::ExportWidget()
    : m_exporter(std::make_unique<VideoExporter>()) {
    strcpy(m_outputFilename, "");
}

void ExportWidget::Reset() {
    if (m_exporter->GetStatus() == ExportStatus::EXPORTING) {
        m_exporter->CancelExport();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    m_exporter->Reset();
    strcpy(m_outputFilename, "");
    m_selectedQuality = ExportQuality::ORIGINAL;
    m_exportLogs.clear();
    m_lastProgress = 0.0f;
    m_initialized = false;
}

void ExportWidget::Draw(const EditingScreen* parent) {
    if (!parent) return;

    DrawDimBackground();

    const VideoInfo& video = parent->GetManager()->GetSelectedVideo();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (!m_initialized) {
        std::string defaultName = GenerateDefaultFilename(video.name);
        strncpy(m_outputFilename, defaultName.c_str(), sizeof(m_outputFilename) - 1);
        m_outputFilename[sizeof(m_outputFilename) - 1] = '\0';
        m_exportLogs.clear();
        m_lastProgress = 0.0f;
        m_initialized = true;
    }

    constexpr auto windowSize = ImVec2(600, 450);
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
        ImGui::Spacing();

        ImGui::Text("Source: %s", video.name.c_str());

        const VideoEditState* editState = parent->GetVideoEditState();
        const float totalDuration = editState->GetTotalDuration();
        const float startTime = editState->GetSelectStart() * totalDuration;
        const float endTime = editState->GetSelectEnd() * totalDuration;

        ImGui::Text("Selection: %.2f s -> %.2f s (Duration: %.2f s)",
                    startTime, endTime, endTime - startTime);

        ImGui::Spacing();

        if (const ExportStatus status = m_exporter->GetStatus(); status == ExportStatus::EXPORTING) {
            const float currentProgress = m_exporter->GetProgress();
            if (currentProgress != m_lastProgress) {
                if (const int percent = static_cast<int>(currentProgress * 100); percent % 5 == 0 && percent != static_cast<int>(m_lastProgress * 100)) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Encoding... %d%% complete", percent);
                    AddLog(msg);
                }
                m_lastProgress = currentProgress;
            }

            ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Export Log:");
            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.07f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

            ImGui::BeginChild("LogWindow", ImVec2(0, 200), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

            for (const auto& log : m_exportLogs) {
                ImGui::TextWrapped("%s", log.c_str());
            }

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();
            ImGui::PopStyleColor(2);

            ImGui::Spacing();

            char progressText[32];
            snprintf(progressText, sizeof(progressText), "%.0f%%", currentProgress * 100.0f);
            ImGui::ProgressBar(currentProgress, ImVec2(-1, 20), progressText);

            ImGui::Spacing();

            if (ImGui::Button("CANCEL EXPORT", ImVec2(-1, 40))) {
                AddLog("Export cancelled by user");
                Reset();
                parent->m_showExportWidget = false;
            }

        } else if (status == ExportStatus::SUCCESS) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "✓ Export completed successfully!");
            ImGui::Text("Saved to:");
            ImGui::TextWrapped("%s", m_exporter->GetOutputPath().c_str());

            ImGui::Spacing();

            if (ImGui::Button("CLOSE", ImVec2(-1, 40))) {
                Reset();
                parent->m_showExportWidget = false;
            }

        } else if (status == ExportStatus::FAILED) {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "✗ Export failed!");
            ImGui::TextWrapped("Error: %s", m_exporter->GetErrorMessage().c_str());

            ImGui::Spacing();

            if (ImGui::Button("CLOSE", ImVec2(-1, 40))) {
                Reset();
                parent->m_showExportWidget = false;
            }

        } else {
            ImGui::Text("Output will be saved to: .../export/");
            ImGui::InputText("Filename", m_outputFilename, sizeof(m_outputFilename));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Maximum File Size:");
            ImGui::Spacing();

            if (ImGui::RadioButton("10 MB", m_selectedQuality == ExportQuality::SIZE_10MB)) {
                m_selectedQuality = ExportQuality::SIZE_10MB;
            }
            ImGui::SameLine();

            if (ImGui::RadioButton("50 MB", m_selectedQuality == ExportQuality::SIZE_50MB)) {
                m_selectedQuality = ExportQuality::SIZE_50MB;
            }
            ImGui::SameLine();

            if (ImGui::RadioButton("100 MB", m_selectedQuality == ExportQuality::SIZE_100MB)) {
                m_selectedQuality = ExportQuality::SIZE_100MB;
            }
            ImGui::SameLine();

            if (ImGui::RadioButton("Original", m_selectedQuality == ExportQuality::ORIGINAL)) {
                m_selectedQuality = ExportQuality::ORIGINAL;
            }

            ImGui::Spacing();

            if (ImGui::Button("EXPORT", ImVec2(220, 45))) {
                m_exportLogs.clear();
                m_lastProgress = 0.0f;

                ExportSettings settings;
                settings.inputPath = video.filePath;
                settings.outputFilename = m_outputFilename;
                settings.startTime = startTime;
                settings.endTime = endTime;

                switch (m_selectedQuality) {
                    case ExportQuality::SIZE_10MB:  settings.maxSizeMB = 10; break;
                    case ExportQuality::SIZE_50MB:  settings.maxSizeMB = 50; break;
                    case ExportQuality::SIZE_100MB: settings.maxSizeMB = 100; break;
                    case ExportQuality::ORIGINAL:   settings.maxSizeMB = 0; break;
                }

                auto logCallback = [this](const std::string& msg) {
                    AddLog(msg);
                };

                m_exporter->StartExport(settings, nullptr, nullptr, logCallback);
            }

            ImGui::SameLine();

            if (ImGui::Button("CANCEL", ImVec2(220, 45))) {
                Reset();
                parent->m_showExportWidget = false;
            }
        }

        if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            Reset();
            parent->m_showExportWidget = false;
        }

        ImGui::End();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
std::string ExportWidget::GenerateDefaultFilename(const std::string& inputFilename) {
    const size_t lastDot = inputFilename.find_last_of('.');

    const std::string basename = (lastDot != std::string::npos)
        ? inputFilename.substr(0, lastDot)
        : inputFilename;

    const std::string extension = (lastDot != std::string::npos)
        ? inputFilename.substr(lastDot)
        : ".mp4";

    return basename + "_trimmed" + extension;
}

void ExportWidget::AddLog(const std::string& message) {
    m_exportLogs.push_back(message);

    if (m_exportLogs.size() > 100) {
        m_exportLogs.erase(m_exportLogs.begin());
    }
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