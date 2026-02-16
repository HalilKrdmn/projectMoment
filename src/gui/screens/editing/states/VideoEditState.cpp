#include <gui/screens/editing/states/VideoEditState.h>

#include "gui/core/MainWindow.h"
#include "gui/screens/editing/EditingScreen.h"
#include "gui/utils/FormatUtils.h"

#include <chrono>
#include <iostream>

#include "imgui.h"

VideoEditState::~VideoEditState() {
}

void VideoEditState::Draw(const EditingScreen *parent) {
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

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_lastFrameTime);
    float deltaTime = duration.count() / 1000.0f;
    m_lastFrameTime = currentTime;

    if (deltaTime > 0.1f) deltaTime = 0.1f;
    if (deltaTime < 0.001f) deltaTime = 0.001f;

    m_videoPlayer->Update(deltaTime);
    m_playbackProgress = static_cast<float>(m_videoPlayer->GetProgress());
    m_isPlaying = m_videoPlayer->IsPlaying();

    DrawInfoBar(parent, video);
    ImGui::Spacing();

    // Video Player
    DrawVideoPlayer();
    ImGui::Spacing();

    // Draw timeline panel
    DrawTimeline(parent, video);
}


void VideoEditState::DrawVideoPlayer() const {
    ImGui::BeginChild("VideoPlayer", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.75f), true);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::GetContentRegionAvail();
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                            IM_COL32(0, 0, 0, 0));

    const float availableHeight = size.y - 20;
    const float availableWidth = size.x - 20;
    const float videoAspect = static_cast<float>(m_videoPlayer->GetWidth()) / m_videoPlayer->GetHeight();

    float displayHeight = availableHeight;
    float displayWidth = displayHeight * videoAspect;

    if (displayWidth > availableWidth) {
        displayWidth = availableWidth;
        displayHeight = displayWidth / videoAspect;
    }

    const float offsetX = (size.x - displayWidth) / 2;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + offsetX, pos.y + 10));
    ImGui::Image(m_videoPlayer->GetFrameTexture(),
                ImVec2(displayWidth, displayHeight),
                ImVec2(0, 0), ImVec2(1, 1));

    ImGui::EndChild();
}

void VideoEditState::DrawTimeline(const EditingScreen* parent, const VideoInfo& video) {
    ImGui::BeginChild("Timeline", ImVec2(0, ImGui::GetContentRegionAvail().y), true);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 containerPos = ImGui::GetCursorScreenPos();
    const ImVec2 containerSize = ImGui::GetContentRegionAvail();

    drawList->AddRectFilled(containerPos, ImVec2(containerPos.x + containerSize.x, containerPos.y + containerSize.y),
                           IM_COL32(0, 0, 0, 0));

    if (!m_audioAnalyzer) {
        ImGui::EndChild();
        return;
    }

    constexpr float topHeight = 50.0f;
    DrawTimelineHeader(parent, ImVec2(containerPos.x, containerPos.y), ImVec2(containerSize.x, topHeight));

    const char* trackNames[] = {"Clip", "Main Audio", "Game Audio", "Microphone"};
    const float trackBoxHeight = (containerSize.y - topHeight) / 4.0f;

    // Draw audio tracks first
    for (int i = 1; i < 4; i++) {
        const float trackY = containerPos.y + topHeight + i * trackBoxHeight;

        const auto trackPos = ImVec2(containerPos.x, trackY);
        const auto trackSize = ImVec2(containerSize.x, trackBoxHeight);

        DrawTrackBoxFull(trackPos, trackSize, trackNames[i], i - 1);
    }

    // Draw Clip track last (On top)
    {
        const float trackY = containerPos.y + topHeight + 0 * trackBoxHeight;

        const auto trackPos = ImVec2(containerPos.x, trackY);
        const auto trackSize = ImVec2(containerSize.x, trackBoxHeight);

        DrawEmptyTrackBoxFull(trackPos, trackSize, trackNames[0]);
    }

    ImGui::EndChild();
}

void VideoEditState::DrawTimelineHeader(const EditingScreen* parent, const ImVec2 pos, const ImVec2 size) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                           IM_COL32(0, 0, 0, 0));


    // Calculates the duration of the video and displays it on the screen.
    const double currentTime = m_videoPlayer->GetCurrentTime();
    const double duration = m_videoPlayer->GetDuration();

    const std::string currentTimeStr = FormatUtils::FormatDuration(currentTime);
    const std::string durationStr = FormatUtils::FormatDuration(duration);
    const std::string timeStr = currentTimeStr + " / " + durationStr;

    drawList->AddText(ImVec2(pos.x + 10, pos.y + 5),
                     IM_COL32(255, 255, 255, 255), timeStr.c_str());


    // Load, Play/Pause/Restart and Next button: Calculates the width of the buttons and places them exactly in the center
    constexpr float buttonsTotalWidth = (50 + 5) + (50 + 5) + 50;
    const float centerX = pos.x + (size.x - buttonsTotalWidth) / 2.0f;

    ImGui::SetCursorScreenPos(ImVec2(centerX, pos.y + 5));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.3f, 0.8f));


    if (ImGui::Button("Prev", ImVec2(50, 35))) {
        // TODO: Load previous video
    }
    ImGui::SameLine(0, 3);


    const char* playPauseLabel;
    if (m_playbackProgress >= 1.0f) {
        playPauseLabel = "Restart";
    } else {
        playPauseLabel = m_isPlaying ? "Pause" : "Play";
    }

    if (ImGui::Button(playPauseLabel, ImVec2(50, 35))) {
        if (m_playbackProgress >= 1.0f) {
            m_videoPlayer->Stop();
            m_playbackProgress = 0.0f;
            m_videoPlayer->Play();
            m_isPlaying = true;
        } else if (m_isPlaying) {
            m_videoPlayer->Pause();
            m_isPlaying = false;
        } else {
            m_videoPlayer->Play();
            m_isPlaying = true;
        }
    }
    ImGui::SameLine(0, 3);


    if (ImGui::Button("Next", ImVec2(50, 35))) {
        // TODO: Load next video
    }
    ImGui::PopStyleColor();

    // Export button: Calculates the width of the buttons and places them on the far right.
    constexpr float exportButtonWidth = 55;
    const float exportX = pos.x + size.x - (exportButtonWidth + 3 + 50) - 10;

    ImGui::SetCursorScreenPos(ImVec2(exportX, pos.y + 5));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.3f, 0.8f));

    if (ImGui::Button("Export", ImVec2(exportButtonWidth, 35))) {
        parent->m_showExportWidget = true;
    }
    ImGui::SameLine(0, 3);

    if (ImGui::Button("...", ImVec2(50, 35))) {
        // TODO: Menu
    }

    ImGui::PopStyleColor();
}

void VideoEditState::DrawEmptyTrackBoxFull(const ImVec2 pos, const ImVec2 size, const char* label) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                           IM_COL32(20, 20, 40, 255));

    // Background label
    drawList->AddText(ImVec2(pos.x + 15, pos.y + size.y / 2 - 8),
                     IM_COL32(200, 200, 200, 255), label);

    // Waveform area
    constexpr float labelWidth = 100.0f;
    const float waveStartX = pos.x + labelWidth;
    const float waveWidth = size.x - labelWidth;
    const float centerY = pos.y + size.y / 2;

    // Draw nothing (empty waveform)
    drawList->AddLine(
        ImVec2(waveStartX, centerY),
        ImVec2(pos.x + size.x, centerY),
        IM_COL32(60, 60, 100, 100),
        1.0f
    );

    // Selection handles on Clip bar
    const float selectStartX = waveStartX + (waveWidth * m_selectStart);
    const float selectEndX = waveStartX + (waveWidth * m_selectEnd);

    // Darkened regions (left of start)
    if (m_selectStart > 0) {
        drawList->AddRectFilled(
            ImVec2(waveStartX, pos.y),
            ImVec2(selectStartX, pos.y + size.y),
            IM_COL32(0, 0, 0, 150)
        );
    }

    // Darkened regions (right of end)
    if (m_selectEnd < 1.0f) {
        drawList->AddRectFilled(
            ImVec2(selectEndX, pos.y),
            ImVec2(waveStartX + waveWidth, pos.y + size.y),
            IM_COL32(0, 0, 0, 150)
        );
    }


    // LEFT HANDLE
    // top triangle - pointing UP
    constexpr float handleSize = 10.0f;
    const ImVec2 leftHandlePoints[] = {
        ImVec2(selectStartX, pos.y + handleSize),    // Top point
        ImVec2(selectStartX - handleSize, pos.y),      // Left point
        ImVec2(selectStartX + handleSize, pos.y)       // Right point
    };
    drawList->AddTriangleFilled(leftHandlePoints[0], leftHandlePoints[1], leftHandlePoints[2],
                               IM_COL32(0, 255, 255, 255));

    // Bottom triangle (inverted - pointing DOWN)
    const ImVec2 leftHandlePointsBottom[] = {
        ImVec2(selectStartX, pos.y + size.y - handleSize),      // Bottom point
        ImVec2(selectStartX - handleSize, pos.y + size.y),      // Left point
        ImVec2(selectStartX + handleSize, pos.y + size.y)       // Right point
    };
    drawList->AddTriangleFilled(leftHandlePointsBottom[0], leftHandlePointsBottom[1], leftHandlePointsBottom[2],
                               IM_COL32(0, 255, 255, 255));

    // Connecting line between triangles
    drawList->AddLine(
        ImVec2(selectStartX, pos.y),
        ImVec2(selectStartX, pos.y + size.y),
        IM_COL32(0, 255, 255, 200),
        3.0f
    );



    // RIGHT HANDLE
    // top triangle - pointing UP
    const ImVec2 rightHandlePoints[] = {
        ImVec2(selectEndX, pos.y + handleSize),      // Top point (up)
        ImVec2(selectEndX - handleSize, pos.y),        // Left point
        ImVec2(selectEndX + handleSize, pos.y)         // Right point
    };
    drawList->AddTriangleFilled(rightHandlePoints[0], rightHandlePoints[1], rightHandlePoints[2],
                               IM_COL32(0, 255, 255, 255));

    // Bottom triangle (inverted - pointing DOWN)
    const ImVec2 rightHandlePointsBottom[] = {
        ImVec2(selectEndX, pos.y + size.y - handleSize),        // Bottom point (down)
        ImVec2(selectEndX - handleSize, pos.y + size.y),        // Left point
        ImVec2(selectEndX + handleSize, pos.y + size.y)         // Right point
    };
    drawList->AddTriangleFilled(rightHandlePointsBottom[0], rightHandlePointsBottom[1], rightHandlePointsBottom[2],
                               IM_COL32(0, 255, 255, 255));

    // Connecting line between triangles
    drawList->AddLine(
        ImVec2(selectEndX, pos.y),
        ImVec2(selectEndX, pos.y + size.y),
        IM_COL32(0, 255, 255, 200),
        3.0f
    );


    // Play line
    const float playlineX = waveStartX + (waveWidth * m_playbackProgress);
    const float bottomY = ImGui::GetWindowPos().y + ImGui::GetWindowSize().y;

    drawList->AddLine(
        ImVec2(playlineX, pos.y),
        ImVec2(playlineX, bottomY),
        IM_COL32(255, 255, 0, 255),
        3.0f
    );

    // Border
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     IM_COL32(60, 60, 100, 255), 1.0f);



    // Bigger handle targets (30px wide, 50px tall)
    // Draggable left handle
    ImGui::SetCursorScreenPos(ImVec2(selectStartX - 15, pos.y - 25));
    ImGui::InvisibleButton("##ClipSelectStartHandle", ImVec2(30, 50));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        const float mouseX = ImGui::GetMousePos().x - waveStartX;
        const float newProgress = mouseX / waveWidth;
        m_selectStart = std::max(0.0f, std::min(newProgress, m_selectEnd - 0.01f));
    }

    // Draggable right handle
    ImGui::SetCursorScreenPos(ImVec2(selectEndX - 15, pos.y - 25));
    ImGui::InvisibleButton("##ClipSelectEndHandle", ImVec2(30, 50));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        const float mouseX = ImGui::GetMousePos().x - waveStartX;
        const float newProgress = mouseX / waveWidth;
        m_selectEnd = std::max(m_selectStart + 0.01f, std::min(1.0f, newProgress));
    }


    // TODO: REPAIR SEEK
    // Click to seek
    ImGui::SetCursorScreenPos(ImVec2(waveStartX, pos.y));
    ImGui::InvisibleButton("##EmptyTrack", ImVec2(waveWidth, size.y));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        const float mouseX = ImGui::GetMousePos().x - waveStartX;
        float clickProgress = mouseX / waveWidth;
        clickProgress = std::max(0.0f, std::min(1.0f, clickProgress));

        m_videoPlayer->Seek(clickProgress * m_videoPlayer->GetDuration());
        m_videoPlayer->Pause();
        m_videoPlayer->Update(0.3f);
    } else if (ImGui::IsItemClicked()) {
        const float mouseX = ImGui::GetMousePos().x - waveStartX;
        float clickProgress = mouseX / waveWidth;
        clickProgress = std::max(0.0f, std::min(1.0f, clickProgress));

        m_videoPlayer->Seek(clickProgress * m_videoPlayer->GetDuration());
        m_videoPlayer->Update(0.3f);
    }
}

void VideoEditState::DrawTrackBoxFull(const ImVec2 pos, const ImVec2 size, const char* label, const int trackIndex) const {
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
    const int totalSeconds = m_audioAnalyzer->GetTotalSeconds();
    if (totalSeconds <= 0) return;

    drawList->AddText(ImVec2(pos.x + 15, pos.y + size.y / 2 - 8),
                     IM_COL32(200, 200, 200, 255), label);

    constexpr float labelWidth = 100.0f;
    const float waveStartX = pos.x + labelWidth;
    const float waveWidth = size.x - labelWidth;
    const float pixelWidth = waveWidth / static_cast<float>(totalSeconds);
    const float centerY = pos.y + size.y / 2;

    for (int second = 0; second < static_cast<int>(waveform.size()); second++) {
        const float value = waveform[second];
        const float barHeight = (size.y / 2) * value;
        const float barX = waveStartX + second * pixelWidth;

        ImU32 color;
        if (trackIndex == 0) color = IM_COL32(100, 200, 255, 255);           // Cyan
        else if (trackIndex == 1) color = IM_COL32(255, 150, 100, 255);      // Orange
        else color = IM_COL32(255, 200, 100, 255);                           // Yellow


        // Top bar
        drawList->AddRectFilled(
            ImVec2(barX, centerY - barHeight),
            ImVec2(barX + pixelWidth - 1, centerY),
            color
        );

        // Bottom bar
        drawList->AddRectFilled(
            ImVec2(barX, centerY),
            ImVec2(barX + pixelWidth - 1, centerY + barHeight),
            color
        );
    }


    const float selectStartX = waveStartX + (waveWidth * m_selectStart);
    const float selectEndX = waveStartX + (waveWidth * m_selectEnd);

    // Darkened regions (left of start)
    if (m_selectStart > 0) {
        drawList->AddRectFilled(
            ImVec2(waveStartX, pos.y),
            ImVec2(selectStartX, pos.y + size.y),
            IM_COL32(0, 0, 0, 150)
        );
    }

    // Darkened regions (right of end)
    if (m_selectEnd < 1.0f) {
        drawList->AddRectFilled(
            ImVec2(selectEndX, pos.y),
            ImVec2(waveStartX + waveWidth, pos.y + size.y),
            IM_COL32(0, 0, 0, 150)
        );
    }


    // Border
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                     IM_COL32(60, 60, 100, 255), 1.0f);


    // TODO: REPAIR SEEK
    // Click and drag
    ImGui::SetCursorScreenPos(ImVec2(waveStartX, pos.y));
    ImGui::InvisibleButton(("##TrackBox" + std::to_string(trackIndex)).c_str(),
                          ImVec2(waveWidth, size.y));

    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        const float mouseX = ImGui::GetMousePos().x - waveStartX;
        float clickProgress = mouseX / waveWidth;
        clickProgress = std::max(0.0f, std::min(1.0f, clickProgress));

        m_videoPlayer->Seek(clickProgress * m_videoPlayer->GetDuration());
        m_videoPlayer->Pause();
        m_videoPlayer->Update(0.033f);
    } else if (ImGui::IsItemClicked()) {
        const float mouseX = ImGui::GetMousePos().x - waveStartX;
        float clickProgress = mouseX / waveWidth;
        clickProgress = std::max(0.0f, std::min(1.0f, clickProgress));

        m_videoPlayer->Seek(clickProgress * m_videoPlayer->GetDuration());
        m_videoPlayer->Update(0.033f);
    }
}



void VideoEditState::DrawInfoBar(const EditingScreen* parent, const VideoInfo& video) const {
    ImGui::BeginChild("InfoBar", ImVec2(0, 40), true);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::GetContentRegionAvail();

    // Background
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                           IM_COL32(40, 40, 80, 255));

    float textX = pos.x + 10;
    const float textY = pos.y + 10;

    // 1. File Name
    const std::string pathLabel = "Nane: " + video.name;
    drawList->AddText(ImVec2(textX, textY), IM_COL32(200, 200, 200, 255), pathLabel.c_str());
    textX += 350;

    // 2. File Size
    char sizeStr[64];
    snprintf(sizeStr, sizeof(sizeStr), "Size: %.1f MB", video.fileSize / (1024.0 * 1024.0));
    drawList->AddText(ImVec2(textX, textY), IM_COL32(150, 200, 150, 255), sizeStr);
    textX += 150;

    // 3. Resolution
    char resStr[64];
    snprintf(resStr, sizeof(resStr), "Resolution: %dx%d", m_videoPlayer->GetWidth(), m_videoPlayer->GetHeight());
    drawList->AddText(ImVec2(textX, textY), IM_COL32(150, 150, 200, 255), resStr);

    // Back button
    ImGui::SetCursorScreenPos(ImVec2(pos.x + size.x - 80, pos.y + (size.y - 20) / 2));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Back", ImVec2(70, 20))) {
        parent->Close();
    }

    ImGui::PopStyleColor(3);

    ImGui::EndChild();
}