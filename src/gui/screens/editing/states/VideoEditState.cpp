#include "gui/screens/editing/states/VideoEditState.h"

#include "gui/screens/editing/EditingScreen.h"

#include "gui/core/MainWindow.h"

#include <chrono>
#include <iostream>

#include "imgui.h"

VideoEditState::~VideoEditState() {
}

void VideoEditState::Draw(EditingScreen *parent) {
    if (!parent) return;

    const VideoInfo& video = parent->GetManager()->GetSelectedVideo();

    if (video.name.empty()) {
        ImGui::Text("No video selected");
        return;
    }

    if (m_videoPlayer && m_lastLoadedPath != video.filePathString) {
        m_videoPlayer = nullptr;
        m_audioAnalyzer = nullptr;
    }

    if (!m_videoPlayer) {
        m_videoPlayer = std::make_unique<VideoPlayer>();
        if (m_videoPlayer->LoadVideo(video.filePathString)) {
            m_audioAnalyzer = std::make_unique<AudioAnalyzer>();
            m_audioAnalyzer->LoadAndComputeTimeline(
                video.filePathString,
                m_videoPlayer->GetDuration());

            m_lastLoadedPath = video.filePathString;
            m_videoPlayer->Play();
            m_isPlaying = true;
            m_lastFrameTime = std::chrono::high_resolution_clock::now();
            std::cout << "[VideoEditState] Loaded: " << video.name << std::endl;
        }
    }

    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_lastFrameTime);
    float deltaTime = duration.count() / 1000.0f;
    m_lastFrameTime = currentTime;

    if (deltaTime > 0.1f) deltaTime = 0.1f;
    if (deltaTime < 0.001f) deltaTime = 0.001f;

    m_videoPlayer->Update(deltaTime);
    m_playbackProgress = static_cast<float>(m_videoPlayer->GetProgress());
    m_isPlaying = m_videoPlayer->IsPlaying();


    ImGui::Columns(2, "TopLayout", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.75f);

    {
        DrawVideoPlayer();
    }

    ImGui::NextColumn();
    {
        DrawInfoCard(video);
        ImGui::Spacing();
        DrawControls(parent);
    }

    ImGui::Columns(1);
    ImGui::Spacing();

    ImGui::Columns(2, "TimelineLayout", false);
    ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 1.0f);

    DrawTimeline(video);

    ImGui::Columns(1);
}


void VideoEditState::DrawVideoPlayer() const {
    ImGui::BeginChild("VideoPlayer", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.75f), true);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                            IM_COL32(220, 80, 80, 255));

    float availableHeight = size.y - 20;
    float availableWidth = size.x - 20;
    float videoAspect = (float)m_videoPlayer->GetWidth() / m_videoPlayer->GetHeight();

    float displayHeight = availableHeight;
    float displayWidth = displayHeight * videoAspect;

    if (displayWidth > availableWidth) {
        displayWidth = availableWidth;
        displayHeight = displayWidth / videoAspect;
    }

    float offsetX = (size.x - displayWidth) / 2;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + offsetX, pos.y + 10));
    ImGui::Image(m_videoPlayer->GetFrameTexture(),
                ImVec2(displayWidth, displayHeight),
                ImVec2(0, 0), ImVec2(1, 1));

    ImGui::EndChild();
}

void VideoEditState::DrawTimeline(const VideoInfo& video) {
    ImGui::BeginChild("Timeline", ImVec2(0, ImGui::GetContentRegionAvail().y), true);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 containerPos = ImGui::GetCursorScreenPos();
    ImVec2 containerSize = ImGui::GetContentRegionAvail();

    drawList->AddRectFilled(containerPos, ImVec2(containerPos.x + containerSize.x, containerPos.y + containerSize.y),
                           IM_COL32(10, 10, 20, 255));

    if (!m_audioAnalyzer) {
        ImGui::EndChild();
        return;
    }

    float topHeight = 50.0f;
    DrawTimelineHeader(ImVec2(containerPos.x, containerPos.y), ImVec2(containerSize.x, topHeight));

    const char* trackNames[] = {"Clip", "Main Audio", "Game Audio", "Microphone"};
    float trackBoxHeight = (containerSize.y - topHeight) / 4.0f;

    for (int i = 0; i < 4; i++) {
        float trackY = containerPos.y + topHeight + i * trackBoxHeight;

        ImVec2 trackPos = ImVec2(containerPos.x, trackY);
        ImVec2 trackSize = ImVec2(containerSize.x, trackBoxHeight);

        if (i == 0) {
            DrawEmptyTrackBoxFull(trackPos, trackSize, trackNames[i]);
        } else {
            DrawTrackBoxFull(trackPos, trackSize, trackNames[i], i - 1);
        }
    }

    ImGui::EndChild();
}

void VideoEditState::DrawTimelineHeader(ImVec2 pos, ImVec2 size) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                           IM_COL32(0, 0, 0, 200));

    double currentTime = m_videoPlayer->GetCurrentTime();
    double duration = m_videoPlayer->GetDuration();

    int currentMin = (int)currentTime / 60;
    int currentSec = (int)currentTime % 60;
    int durationMin = (int)duration / 60;
    int durationSec = (int)duration % 60;

    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d / %02d:%02d",
             currentMin, currentSec, durationMin, durationSec);

    drawList->AddText(ImVec2(pos.x + 10, pos.y + 5),
                     IM_COL32(255, 255, 255, 255), timeStr);

    ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x - 200, pos.y + 5));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.3f, 0.8f));

    if (ImGui::Button("Play", ImVec2(50, 35))) {
        m_videoPlayer->Play();
        m_isPlaying = true;
    }
    ImGui::SameLine(0, 5);
    if (ImGui::Button("Pause", ImVec2(50, 35))) {
        m_videoPlayer->Pause();
        m_isPlaying = false;
    }
    ImGui::SameLine(0, 5);
    if (ImGui::Button("Stop", ImVec2(50, 35))) {
        m_videoPlayer->Stop();
        m_isPlaying = false;
        m_playbackProgress = 0.0f;
    }

    ImGui::PopStyleColor();
}

void VideoEditState::DrawEmptyTrackBoxFull(const ImVec2 pos, ImVec2 size, const char* label) const {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                           IM_COL32(20, 20, 40, 255));

    drawList->AddText(ImVec2(pos.x + 15, pos.y + size.y / 2 - 8),
                     IM_COL32(200, 200, 200, 255), label);

    float labelWidth = 100.0f;
    float waveStartX = pos.x + labelWidth;
    float waveWidth = size.x - labelWidth;
    float centerY = pos.y + size.y / 2;

    drawList->AddLine(
        ImVec2(waveStartX, centerY),
        ImVec2(pos.x + size.x, centerY),
        IM_COL32(60, 60, 100, 100),
        1.0f
    );

    const float x = waveStartX + (waveWidth * m_playbackProgress);
    drawList->AddLine(
        ImVec2(x, pos.y),
        ImVec2(x, pos.y + size.y),
        IM_COL32(255, 255, 0, 255),
        2.0f
    );

    // Border
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     IM_COL32(60, 60, 100, 255), 1.0f);

    // Click to seek
    ImGui::SetCursorScreenPos(ImVec2(waveStartX, pos.y));
    ImGui::InvisibleButton("##EmptyTrack", ImVec2(waveWidth, size.y));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        float mouseX = ImGui::GetMousePos().x - waveStartX;
        float clickProgress = mouseX / waveWidth;
        clickProgress = std::max(0.0f, std::min(1.0f, clickProgress));

        m_videoPlayer->Seek(clickProgress * m_videoPlayer->GetDuration());
        m_videoPlayer->Play();
    } else if (ImGui::IsItemClicked()) {
        float mouseX = ImGui::GetMousePos().x - waveStartX;
        float clickProgress = mouseX / waveWidth;
        clickProgress = std::max(0.0f, std::min(1.0f, clickProgress));

        m_videoPlayer->Seek(clickProgress * m_videoPlayer->GetDuration());
    }
}

void VideoEditState::DrawTrackBoxFull(ImVec2 pos, ImVec2 size, const char* label, const int trackIndex) const {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                           IM_COL32(20, 20, 40, 255));

    if (!m_audioAnalyzer) {
        drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                         IM_COL32(100, 100, 150, 255), 1.0f);
        return;
    }

    if (const auto& tracks = m_audioAnalyzer->GetTracks(); trackIndex < 0 || trackIndex >= static_cast<int>(tracks.size())) {
        drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                         IM_COL32(100, 100, 150, 255), 1.0f);
        return;
    }

    const auto& waveform = m_audioAnalyzer->GetWaveform(trackIndex);
    int totalSeconds = m_audioAnalyzer->GetTotalSeconds();
    if (totalSeconds <= 0) return;

    drawList->AddText(ImVec2(pos.x + 15, pos.y + size.y / 2 - 8),
                     IM_COL32(200, 200, 200, 255), label);

    float labelWidth = 100.0f;
    float waveStartX = pos.x + labelWidth;
    float waveWidth = size.x - labelWidth;
    float pixelWidth = waveWidth / (float)totalSeconds;
    float centerY = pos.y + size.y / 2;

    for (int second = 0; second < (int)waveform.size(); second++) {
        float value = waveform[second];
        float barHeight = (size.y / 2) * value;
        float barX = waveStartX + second * pixelWidth;

        ImU32 color;
        if (trackIndex == 0) color = IM_COL32(100, 200, 255, 255);  // Cyan
        else if (trackIndex == 1) color = IM_COL32(255, 150, 100, 255);  // Orange
        else color = IM_COL32(255, 200, 100, 255);  // Yellow

        // Top bar (center up)
        drawList->AddRectFilled(
            ImVec2(barX, centerY - barHeight),
            ImVec2(barX + pixelWidth - 1, centerY),
            color
        );

        // Bottom bar (center down)
        drawList->AddRectFilled(
            ImVec2(barX, centerY),
            ImVec2(barX + pixelWidth - 1, centerY + barHeight),
            color
        );
    }

    const float x = waveStartX + (waveWidth * m_playbackProgress);
    drawList->AddLine(
        ImVec2(x, pos.y),
        ImVec2(x, pos.y + size.y),
        IM_COL32(255, 255, 0, 255),
        2.0f
    );

    // Border
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     IM_COL32(60, 60, 100, 255), 1.0f);

    // Click and drag
    ImGui::SetCursorScreenPos(ImVec2(waveStartX, pos.y));
    ImGui::InvisibleButton(("##TrackBox" + std::to_string(trackIndex)).c_str(),
                          ImVec2(waveWidth, size.y));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        float mouseX = ImGui::GetMousePos().x - waveStartX;
        float clickProgress = mouseX / waveWidth;
        clickProgress = std::max(0.0f, std::min(1.0f, clickProgress));

        m_videoPlayer->Seek(clickProgress * m_videoPlayer->GetDuration());
        m_videoPlayer->Play();
    } else if (ImGui::IsItemClicked()) {
        float mouseX = ImGui::GetMousePos().x - waveStartX;
        float clickProgress = mouseX / waveWidth;
        clickProgress = std::max(0.0f, std::min(1.0f, clickProgress));

        m_videoPlayer->Seek(clickProgress * m_videoPlayer->GetDuration());
    }
}

void VideoEditState::DrawInfoCard(const VideoInfo& video) {
    ImGui::BeginChild("VideoInfo", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.50f), true);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();

    // Background green
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                            IM_COL32(100, 200, 100, 255));

    // Border
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     IM_COL32(80, 180, 80, 255), 2.0f);

    // Title
    drawList->AddText(ImVec2(pos.x + 8, pos.y + 10),
                     IM_COL32(0, 0, 0, 255), "Info");

    ImVec2 contentPos = ImVec2(pos.x + 8, pos.y + 35);
    float textY = contentPos.y;

    // Name
    drawList->AddText(ImVec2(contentPos.x, textY),
                     IM_COL32(0, 0, 0, 255), "Name:");
    std::string shortName = video.name;
    if (shortName.length() > 12) shortName = shortName.substr(0, 12) + "...";
    drawList->AddText(ImVec2(contentPos.x, textY + 12),
                     IM_COL32(30, 30, 30, 255), shortName.c_str());
    textY += 30;

    // Duration
    char durationStr[40];
    snprintf(durationStr, sizeof(durationStr), "Dur: %.1fs",
             m_videoPlayer->GetDuration());
    drawList->AddText(ImVec2(contentPos.x, textY),
                     IM_COL32(0, 0, 0, 255), durationStr);
    textY += 20;

    // Resolution
    char resStr[32];
    snprintf(resStr, sizeof(resStr), "%dx%d",
             m_videoPlayer->GetWidth(), m_videoPlayer->GetHeight());
    drawList->AddText(ImVec2(contentPos.x, textY),
                     IM_COL32(0, 0, 0, 255), resStr);
    textY += 20;

    // Tracks
    if (m_audioAnalyzer) {
        const auto& tracks = m_audioAnalyzer->GetTracks();
        char trackStr[32];
        snprintf(trackStr, sizeof(trackStr), "Tracks: %zu", tracks.size());
        drawList->AddText(ImVec2(contentPos.x, textY),
                         IM_COL32(0, 0, 0, 255), trackStr);
    }

    ImGui::EndChild();
}

void VideoEditState::DrawControls(EditingScreen* parent) {
    ImGui::BeginChild("ExportButtons", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.45f), true);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                           IM_COL32(40, 40, 80, 255));

    // Buttons
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 10, pos.y + 10));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 0.8f));

    float buttonWidth = size.x - 20;

    if (ImGui::Button("📤 Export", ImVec2(buttonWidth, 40))) {
        if (parent) {
            parent->ChangeState(EditingScreenState::EXPORT);
        }
    }

    if (ImGui::Button("◀ Back", ImVec2(buttonWidth, 40))) {
        if (parent) {
            parent->GetManager()->SetApplicationState(ApplicationState::MAIN);
        }
    }

    ImGui::PopStyleColor();
    ImGui::EndChild();
}
