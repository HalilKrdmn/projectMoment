#include <gui/screens/editing/states/VideoEditState.h>

#include "gui/Theme.h"
#include "gui/core/MainWindow.h"
#include "gui/screens/editing/EditingScreen.h"
#include "gui/utils/FormatUtils.h"
#include "core/CoreServices.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "imgui.h"

static ImU32 TrackWaveColor(int i) {
    switch (i % 5) {
        case 0:  return IM_COL32(100, 200, 255, 255);
        case 1:  return IM_COL32(255, 150, 100, 255);
        case 2:  return IM_COL32(180, 130, 255, 255);
        case 3:  return IM_COL32(255, 210,  80, 255);
        default: return IM_COL32(130, 220, 160, 255);
    }
}
static ImU32 TrackBgColor(int i) {
    switch (i % 5) {
        case 0:  return IM_COL32(18, 28, 42, 255);
        case 1:  return IM_COL32(32, 22, 18, 255);
        case 2:  return IM_COL32(24, 18, 38, 255);
        case 3:  return IM_COL32(32, 28, 14, 255);
        default: return IM_COL32(18, 32, 22, 255);
    }
}

VideoEditState::~VideoEditState() {
    {
        std::lock_guard lock(m_analyzerMutex);
        m_pendingAnalyzer.reset();
        m_audioAnalyzer.reset();
    }
    m_videoPlayer.reset();
}

void VideoEditState::Draw(const EditingScreen* parent) {
    if (!parent) return;

    const VideoInfo& video = parent->GetManager()->GetSelectedVideo();
    if (video.name.empty()) { ImGui::Text("No video selected"); return; }

    if (m_videoPlayer && m_lastLoadedPath != video.filePathString) {
        m_videoPlayer.reset();
        std::lock_guard lock(m_analyzerMutex);
        m_audioAnalyzer.reset();
        m_pendingAnalyzer.reset();
    }

    if (!m_videoPlayer) {
        m_videoPlayer = std::make_unique<VideoPlayer>();
        if (m_videoPlayer->LoadVideo(video.filePathString)) {
            const double dur   = m_videoPlayer->GetDuration();
            const std::string path = video.filePathString;

            std::thread([this, path, dur]() {
                auto analyzer = std::make_unique<AudioAnalyzer>();
                try {
                    if (!analyzer->LoadAndComputeTimeline(path, dur)) {
                        std::cerr << "[VideoEditState] Analyzer failed" << std::endl;
                        return;
                    }
                } catch (const std::bad_alloc& e) {
                    std::cerr << "[VideoEditState] bad_alloc: " << e.what() << std::endl;
                    return;
                } catch (...) {
                    std::cerr << "[VideoEditState] unknown exception in analyzer" << std::endl;
                    return;
                }
                std::lock_guard<std::mutex> lock(m_analyzerMutex);
                m_pendingAnalyzer = std::move(analyzer);
            }).detach();

            m_lastLoadedPath = video.filePathString;
            m_videoPlayer->Play();
            m_isPlaying     = true;
            m_lastFrameTime = std::chrono::high_resolution_clock::now();
            std::cout << "[VideoEditState] Loaded: " << video.name << std::endl;
        } else {
            m_videoPlayer.reset();
            return;
        }
    }

    // Swap pending → active (main thread only)
    {
        std::lock_guard lock(m_analyzerMutex);
        if (m_pendingAnalyzer) {
            m_audioAnalyzer  = std::move(m_pendingAnalyzer);
            m_pendingAnalyzer = nullptr;
        }
    }

    const auto now    = std::chrono::high_resolution_clock::now();
    float deltaTime   = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - m_lastFrameTime).count() / 1000.0f;
    m_lastFrameTime   = now;
    deltaTime         = std::clamp(deltaTime, 0.001f, 0.1f);

    m_videoPlayer->Update(deltaTime);
    m_playbackProgress = (float)m_videoPlayer->GetProgress();
    m_isPlaying        = m_videoPlayer->IsPlaying();

    DrawInfoBar(parent, video);
    ImGui::Spacing();
    DrawVideoPlayer();
    ImGui::Spacing();
    DrawTimeline(parent, video);
}

void VideoEditState::DrawVideoPlayer() const {
    ImGui::BeginChild("VideoPlayer",
                      ImVec2(0, ImGui::GetContentRegionAvail().y * 0.75f), false);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos(), size = ImGui::GetContentRegionAvail();
    dl->AddRectFilled(pos, ImVec2(pos.x+size.x, pos.y+size.y), IM_COL32(0,0,0,0));

    const float avH = size.y-20, avW = size.x-20;
    const float ar  = (float)m_videoPlayer->GetWidth() / (float)m_videoPlayer->GetHeight();
    float dW = avH*ar, dH = avH;
    if (dW > avW) { dW = avW; dH = dW/ar; }

    ImGui::SetCursorScreenPos(ImVec2(pos.x+(size.x-dW)/2, pos.y+10));
    ImGui::Image(m_videoPlayer->GetFrameTexture(), ImVec2(dW,dH), ImVec2(0,0), ImVec2(1,1));
    ImGui::EndChild();
}

void VideoEditState::DrawTimeline(const EditingScreen* parent, const VideoInfo&) {
    const ImVec2 tlAvail = ImGui::GetContentRegionAvail();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::BeginChild("Timeline", ImVec2(tlAvail.x - 20.0f, tlAvail.y), false);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 cPos = ImGui::GetCursorScreenPos(), cSize = ImGui::GetContentRegionAvail();
    dl->AddRectFilled(cPos, ImVec2(cPos.x+cSize.x, cPos.y+cSize.y), IM_COL32(0,0,0,0));

    constexpr float HPAD = 10.0f;
    cPos.x  += HPAD;
    cSize.x -= HPAD * 2.0f;

    constexpr float headerH = 50.0f;
    DrawTimelineHeader(parent, cPos, ImVec2(cSize.x, headerH));

    const Config* cfg = CoreServices::Instance().GetConfig();
    struct DisplayTrack { std::string name; int analyzerIdx; };
    std::vector<DisplayTrack> displayTracks;

    if (m_audioAnalyzer) {
        const int fileTrackCount = m_audioAnalyzer->GetTrackCount();

        // Collect configured track names from config (if available)
        std::vector<std::string> cfgNames;
        if (cfg) {
            for (const auto& t : cfg->nativeAudioTracks)
                cfgNames.push_back(t.name.empty() ? t.device : t.name);
        }

        // Use whichever is larger: file tracks or configured tracks
        const int totalRows = std::max(fileTrackCount, static_cast<int>(cfgNames.size()));
        for (int i = 0; i < totalRows; i++) {
            DisplayTrack dt;
            // Name: prefer config name, fall back to analyzer name
            if (i < static_cast<int>(cfgNames.size()) && !cfgNames[i].empty())
                dt.name = cfgNames[i];
            else if (i < fileTrackCount)
                dt.name = m_audioAnalyzer->GetTracks()[i].name;
            else
                dt.name = "Audio Track " + std::to_string(i + 1);

            dt.analyzerIdx = (i < fileTrackCount) ? i : -1;
            displayTracks.push_back(std::move(dt));
        }
    } else {
        // Analyzer not ready yet — show config tracks as loading placeholders
        if (cfg) {
            for (const auto& t : cfg->nativeAudioTracks) {
                DisplayTrack dt;
                dt.name        = t.name.empty() ? t.device : t.name;
                dt.analyzerIdx = -2;
                displayTracks.push_back(std::move(dt));
            }
        }
        if (displayTracks.empty()) {
            displayTracks.push_back({"Audio", -2});
        }
    }

    const int totalRows  = 1 + static_cast<int>(displayTracks.size());
    const float trackH   = (cSize.y - headerH) / static_cast<float>(totalRows);

    // Row 0: Clip (video)
    DrawEmptyTrackBoxFull(ImVec2(cPos.x, cPos.y+headerH), ImVec2(cSize.x, trackH), "Clip");

    // Audio rows
    for (int i = 0; i < (int)displayTracks.size(); i++) {
        ImVec2 tp = ImVec2(cPos.x, cPos.y + headerH + (i+1) * trackH);
        ImVec2 ts = ImVec2(cSize.x, trackH);
        const auto& dt = displayTracks[i];

        if (dt.analyzerIdx == -2) {
            // Loading placeholder
            dl->AddRectFilled(tp, ImVec2(tp.x+ts.x, tp.y+ts.y), IM_COL32(20,20,40,255));
            dl->AddText(ImVec2(tp.x+15, tp.y+ts.y/2-8),
                        IM_COL32(120,120,160,200), dt.name.c_str());
            const std::string loadStr = "  (loading...)";
            dl->AddText(ImVec2(tp.x+15+ImGui::CalcTextSize(dt.name.c_str()).x,
                               tp.y+ts.y/2-8),
                        IM_COL32(80,80,100,180), loadStr.c_str());
            dl->AddRect(tp, ImVec2(tp.x+ts.x, tp.y+ts.y), IM_COL32(60,60,100,255));
        } else {
            DrawTrackBoxFull(tp, ts, dt.name.c_str(), dt.analyzerIdx);
        }
    }

    ImGui::EndChild();
}

void VideoEditState::DrawTimelineHeader(const EditingScreen* parent,
                                        const ImVec2 pos, const ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x+size.x, pos.y+size.y), IM_COL32(0,0,0,0));

    std::string ts = FormatUtils::FormatDuration(m_videoPlayer->GetCurrentTime())
                   + " / "
                   + FormatUtils::FormatDuration(m_videoPlayer->GetDuration());
    dl->AddText(ImVec2(pos.x+10, pos.y+5), IM_COL32(255,255,255,255), ts.c_str());

    constexpr float bW=50, bG=5, totW=3*bW+2*bG;
    ImGui::SetCursorScreenPos(ImVec2(pos.x+(size.x-totW)/2, pos.y+5));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f,0.2f,0.3f,0.8f));

    if (ImGui::Button("Prev", ImVec2(bW,35))) {}
    ImGui::SameLine(0,bG);

    const char* ppLbl = (m_playbackProgress>=1.f) ? "Restart"
                       : (m_isPlaying ? "Pause" : "Play");
    if (ImGui::Button(ppLbl, ImVec2(bW,35))) {
        if (m_playbackProgress >= 1.f) {
            m_videoPlayer->Stop(); m_playbackProgress=0; m_videoPlayer->Play(); m_isPlaying=true;
        } else if (m_isPlaying) {
            m_videoPlayer->Pause(); m_isPlaying=false;
        } else {
            m_videoPlayer->Play(); m_isPlaying=true;
        }
    }
    ImGui::SameLine(0,bG);
    if (ImGui::Button("Next", ImVec2(bW,35))) {}
    ImGui::PopStyleColor();

    constexpr float expW=55;
    ImGui::SetCursorScreenPos(ImVec2(pos.x+size.x-(expW+bG+bW)-10, pos.y+5));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f,0.5f,0.3f,0.8f));
    if (ImGui::Button("Export", ImVec2(expW,35))) parent->m_showExportWidget=true;
    ImGui::SameLine(0,bG);
    if (ImGui::Button("...", ImVec2(bW,35))) {}
    ImGui::PopStyleColor();
}

void VideoEditState::DrawEmptyTrackBoxFull(ImVec2 pos, ImVec2 size, const char* label) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x+size.x, pos.y+size.y), IM_COL32(20,20,40,255));
    dl->AddText(ImVec2(pos.x+15, pos.y+size.y/2-8), IM_COL32(200,200,200,255), label);

    const float wx=pos.x+100, ww=size.x-100, cy=pos.y+size.y/2;
    dl->AddLine(ImVec2(wx,cy), ImVec2(wx+ww,cy), IM_COL32(60,60,100,100), 1);

    const float sx=wx+ww*m_selectStart, ex=wx+ww*m_selectEnd;
    if (m_selectStart>0) dl->AddRectFilled(ImVec2(wx,pos.y), ImVec2(sx,pos.y+size.y), IM_COL32(0,0,0,150));
    if (m_selectEnd<1)   dl->AddRectFilled(ImVec2(ex,pos.y), ImVec2(wx+ww,pos.y+size.y), IM_COL32(0,0,0,150));

    auto drawHandle = [&](float hx) {
        constexpr float hs=10;
        dl->AddTriangleFilled(ImVec2(hx,pos.y+hs), ImVec2(hx-hs,pos.y), ImVec2(hx+hs,pos.y), IM_COL32(0,255,255,255));
        dl->AddTriangleFilled(ImVec2(hx,pos.y+size.y-hs), ImVec2(hx-hs,pos.y+size.y), ImVec2(hx+hs,pos.y+size.y), IM_COL32(0,255,255,255));
        dl->AddLine(ImVec2(hx,pos.y), ImVec2(hx,pos.y+size.y), IM_COL32(0,255,255,200), 3);
    };
    drawHandle(sx); drawHandle(ex);

    const float px=wx+ww*m_playbackProgress;
    dl->AddLine(ImVec2(px,pos.y), ImVec2(px, ImGui::GetWindowPos().y+ImGui::GetWindowSize().y), IM_COL32(255,255,0,255), 3);
    dl->AddRect(pos, ImVec2(pos.x+size.x, pos.y+size.y), IM_COL32(60,60,100,255));

    ImGui::SetCursorScreenPos(ImVec2(sx-15, pos.y-25));
    ImGui::InvisibleButton("##SelS", ImVec2(30,50));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        m_selectStart = std::max(0.f, std::min((ImGui::GetMousePos().x-wx)/ww, m_selectEnd-0.01f));

    ImGui::SetCursorScreenPos(ImVec2(ex-15, pos.y-25));
    ImGui::InvisibleButton("##SelE", ImVec2(30,50));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        m_selectEnd = std::max(m_selectStart+0.01f, std::min(1.f, (ImGui::GetMousePos().x-wx)/ww));

    ImGui::SetCursorScreenPos(ImVec2(wx, pos.y));
    ImGui::InvisibleButton("##EmptyTrack", ImVec2(ww, size.y));
    auto doSeek = [&]() {
        float p = std::clamp((ImGui::GetMousePos().x-wx)/ww, 0.f, 1.f);
        m_videoPlayer->Seek(p * m_videoPlayer->GetDuration());
        m_videoPlayer->Update(0.033f);
    };
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) { m_videoPlayer->Pause(); doSeek(); }
    else if (ImGui::IsItemClicked()) doSeek();
}

void VideoEditState::DrawTrackBoxFull(ImVec2 pos, ImVec2 size, const char* label, int ti) const {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // ti == -1: track exists in config but not in the recorded file (mixed/missing)
    const bool noStream = (ti < 0);
    const bool silent   = noStream || (m_audioAnalyzer && m_audioAnalyzer->IsTrackSilent(ti));

    dl->AddRectFilled(pos, ImVec2(pos.x+size.x, pos.y+size.y),
                      silent ? IM_COL32(12,12,18,255) : TrackBgColor(ti));
    dl->AddText(ImVec2(pos.x+15, pos.y+size.y/2-8),
                silent ? IM_COL32(80,80,100,255) : IM_COL32(200,200,200,255), label);

    if (noStream) {
        dl->AddText(ImVec2(pos.x+15, pos.y+size.y/2+6),
                    IM_COL32(70,70,90,200), "- NOT IN FILE (mixed) -");
    } else if (silent) {
        dl->AddText(ImVec2(pos.x+15, pos.y+size.y/2+6),
                    IM_COL32(80,80,100,200), "- SILENT -");
    }

    const float wx=pos.x+100, ww=size.x-100, cy=pos.y+size.y/2;

    if (!noStream && m_audioAnalyzer && !silent && ti < m_audioAnalyzer->GetTrackCount()) {
        const auto& wf  = m_audioAnalyzer->GetWaveform(ti);
        const int tsec  = m_audioAnalyzer->GetTotalSeconds();
        if (tsec > 0) {
            const float pps = ww / static_cast<float>(tsec);
            const ImU32 col = TrackWaveColor(ti);
            for (int s = 0; s < static_cast<int>(wf.size()); s++) {
                const float bh=size.y*0.45f*wf[s], bx=wx+s*pps;
                dl->AddRectFilled(ImVec2(bx,cy-bh), ImVec2(bx+pps-1,cy), col);
                dl->AddRectFilled(ImVec2(bx,cy),    ImVec2(bx+pps-1,cy+bh), col);
            }
        }
    } else {
        dl->AddLine(ImVec2(wx,cy), ImVec2(wx+ww,cy), IM_COL32(50,50,70,160), 1);
    }

    const float sx=wx+ww*m_selectStart, ex=wx+ww*m_selectEnd;
    if (m_selectStart>0) dl->AddRectFilled(ImVec2(wx,pos.y), ImVec2(sx,pos.y+size.y), IM_COL32(0,0,0,130));
    if (m_selectEnd<1)   dl->AddRectFilled(ImVec2(ex,pos.y), ImVec2(wx+ww,pos.y+size.y), IM_COL32(0,0,0,130));
    dl->AddRect(pos, ImVec2(pos.x+size.x, pos.y+size.y), IM_COL32(60,60,100,255));

    // Seek only when we have real data
    if (!noStream) {
        ImGui::SetCursorScreenPos(ImVec2(wx, pos.y));
        ImGui::InvisibleButton(("##TB"+std::to_string(ti)).c_str(), ImVec2(ww, size.y));
        auto doSeek = [&]() {
            float p = std::clamp((ImGui::GetMousePos().x-wx)/ww, 0.f, 1.f);
            m_videoPlayer->Seek(p * m_videoPlayer->GetDuration());
            m_videoPlayer->Update(0.033f);
        };
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) { m_videoPlayer->Pause(); doSeek(); }
        else if (ImGui::IsItemClicked()) doSeek();
    }
}

void VideoEditState::DrawInfoBar(const EditingScreen* parent, const VideoInfo& video) const {
    const ImGuiViewport* vp    = ImGui::GetMainViewport();
    const float          totalW = vp->WorkSize.x;

    // Background + divider line via draw lists
    {
        ImDrawList* bg = ImGui::GetBackgroundDrawList();
        const ImVec2 p0 = vp->WorkPos;
        const ImVec2 p1 = { p0.x + totalW, p0.y + Theme::TOPBAR_H };
        bg->AddRectFilled(p0, p1, ImGui::GetColorU32(Theme::BG_DARK));
    }
    {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const ImVec2 p0 = vp->WorkPos;
        const float  lineY = p0.y + Theme::TOPBAR_H;
        fg->AddLine({ p0.x, lineY }, { p0.x + totalW, lineY }, Theme::SEPARATOR_LINE, 1.0f);
    }

    // Child window for widgets (transparent over the bg rect)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("InfoBar", ImVec2(totalW, Theme::TOPBAR_H), false,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();

    //Info text (left, vertically centered)
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2      pos = ImGui::GetWindowPos();
    const Config* cfg = CoreServices::Instance().GetConfig();

    const float ty = pos.y + (Theme::TOPBAR_H - ImGui::GetTextLineHeight()) * 0.5f;
    float tx = pos.x + 24.0f;

    dl->AddText(ImVec2(tx, ty), ImGui::GetColorU32(Theme::TEXT_PRIMARY),
                ("Name: " + video.name).c_str());
    tx += 350.0f;

    char buf[128];
    snprintf(buf, sizeof(buf), "Size: %.1f MB", video.fileSize / (1024.0 * 1024.0));
    dl->AddText(ImVec2(tx, ty), ImGui::GetColorU32(Theme::TEXT_PRIMARY), buf);
    tx += 150.0f;

    snprintf(buf, sizeof(buf), "Res: %dx%d",
             m_videoPlayer->GetWidth(), m_videoPlayer->GetHeight());
    dl->AddText(ImVec2(tx, ty), ImGui::GetColorU32(Theme::TEXT_PRIMARY), buf);
    tx += 160.0f;

    if (m_audioAnalyzer || cfg) {
        int cfgTrackCount  = cfg ? (int)cfg->nativeAudioTracks.size() : 0;
        int fileTrackCount = m_audioAnalyzer ? m_audioAnalyzer->GetTrackCount() : 0;
        int displayCount   = std::max(cfgTrackCount, fileTrackCount);
        if (displayCount > 0) {
            snprintf(buf, sizeof(buf), "Audio: %d track(s)", displayCount);
            dl->AddText(ImVec2(tx, ty), ImGui::GetColorU32(Theme::TEXT_PRIMARY), buf);
        }
    }

    constexpr float btnY = (Theme::TOPBAR_H - Theme::TOPBAR_BTN_H) * 0.5f;
    ImGui::SetCursorPos(ImVec2(totalW - Theme::TOPBAR_BTN_PAD - Theme::TOPBAR_BTN_W - 10.0f, btnY));
    ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::DANGER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::DANGER);
    if (ImGui::Button("X", ImVec2(Theme::TOPBAR_BTN_W, Theme::TOPBAR_BTN_H))) parent->Close();
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::PopStyleColor(); // ChildBg
}