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

// ─── Layout constants ─────────────────────────────────────────────────────────
static constexpr float HEADER_H    = 50.0f;  // timeline header (time + controls)
static constexpr float TRACK_H     = 50.0f;  // each track row height
static constexpr float BOTTOM_PAD  = 10.0f;  // gap below last track
static constexpr int   MIN_AUDIO   = 3;      // always show at least 3 audio rows
static constexpr float LABEL_W     = 110.0f; // label column width (includes right padding)

// ─── Color helpers ────────────────────────────────────────────────────────────
static ImU32 WaveColor(const AudioDeviceType dt) {
    if (dt == AudioDeviceType::Input)  return Theme::TL_WAVE_INPUT;
    if (dt == AudioDeviceType::Output) return Theme::TL_WAVE_OUTPUT;
    return Theme::TL_WAVE_UNKNOWN;
}
static ImU32 BgColor(const AudioDeviceType dt) {
    if (dt == AudioDeviceType::Input)  return Theme::TL_INPUT_BG;
    if (dt == AudioDeviceType::Output) return Theme::TL_OUTPUT_BG;
    return Theme::TL_UNKNOWN_BG;
}

static void SplitLabel(const std::string& label, int maxLen,
                        std::string& line1, std::string& line2)
{
    if (static_cast<int>(label.size()) <= maxLen) { line1 = label; line2 = ""; return; }
    // Try to split at a space near maxLen
    size_t sp = label.rfind(' ', maxLen);
    if (sp == std::string::npos || sp == 0) sp = maxLen;
    line1 = label.substr(0, sp);
    line2 = label.substr(sp + (label[sp] == ' ' ? 1 : 0));
    // Truncate line2 with "..." if still too long
    if (static_cast<int>(line2.size()) > maxLen) line2 = line2.substr(0, maxLen - 3) + "...";
}

// ──────────────────────────────────────────────────────────────────────────────
VideoEditState::~VideoEditState() {
    {
        std::lock_guard lock(m_analyzerMutex);
        m_pendingAnalyzer.reset();
        m_audioAnalyzer.reset();
    }
    m_videoPlayer.reset();
}

// ──────────────────────────────────────────────────────────────────────────────
float VideoEditState::ComputeTimelineHeight() const {
    int audioRows = MIN_AUDIO;
    const Config* cfg = CoreServices::Instance().GetConfig();
    if (cfg)               audioRows = std::max(audioRows, static_cast<int>(cfg->nativeAudioTracks.size()));
    if (m_audioAnalyzer)   audioRows = std::max(audioRows, m_audioAnalyzer->GetTrackCount());
    return HEADER_H + (1 + audioRows) * TRACK_H + BOTTOM_PAD;
}

// ─── Draw ─────────────────────────────────────────────────────────────────
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
            const double dur  = m_videoPlayer->GetDuration();
            const std::string path = video.filePathString;

            std::thread([this, path, dur]() {
                auto analyzer = std::make_unique<AudioAnalyzer>();
                try {
                    if (!analyzer->LoadAndComputeTimeline(path, dur)) {
                        std::cerr << "[VideoEditState] Analyzer failed\n";
                        return;
                    }
                } catch (const std::bad_alloc& e) {
                    std::cerr << "[VideoEditState] bad_alloc: " << e.what() << "\n";
                    return;
                } catch (...) {
                    std::cerr << "[VideoEditState] unknown exception in analyzer\n";
                    return;
                }
                std::lock_guard<std::mutex> lock(m_analyzerMutex);
                m_pendingAnalyzer = std::move(analyzer);
            }).detach();

            m_lastLoadedPath = video.filePathString;
            m_videoPlayer->Play();
            m_isPlaying     = true;
            m_lastFrameTime = std::chrono::high_resolution_clock::now();
            std::cout << "[VideoEditState] Loaded: " << video.name << "\n";
        } else {
            m_videoPlayer.reset();
            return;
        }
    }

    // Swap pending → active (main thread only)
    {
        std::lock_guard lock(m_analyzerMutex);
        if (m_pendingAnalyzer) {
            m_audioAnalyzer   = std::move(m_pendingAnalyzer);
            m_pendingAnalyzer = nullptr;
        }
    }

    const auto now   = std::chrono::high_resolution_clock::now();
    float deltaTime  = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now - m_lastFrameTime).count() / 1000.0f;
    m_lastFrameTime  = now;
    deltaTime        = std::clamp(deltaTime, 0.001f, 0.1f);

    m_videoPlayer->Update(deltaTime);
    m_playbackProgress = static_cast<float>(m_videoPlayer->GetProgress());
    m_isPlaying        = m_videoPlayer->IsPlaying();

    const float tlH = ComputeTimelineHeight();

    DrawInfoBar(parent, video);
    ImGui::Spacing();
    DrawVideoPlayer(tlH);
    ImGui::Spacing();
    DrawTimeline(parent, video);
}

void VideoEditState::DrawVideoPlayer(const float reservedTimelineH) const {
    const float spacing = ImGui::GetStyle().ItemSpacing.y * 2.0f;
    const float vpH     = std::max(80.0f,
        ImGui::GetContentRegionAvail().y - reservedTimelineH - spacing);

    ImGui::BeginChild("VideoPlayer", ImVec2(0, vpH), false);
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2 pos  = ImGui::GetCursorScreenPos();
    ImVec2 size = ImGui::GetContentRegionAvail();
    dl->AddRectFilled(pos, ImVec2(pos.x+size.x, pos.y+size.y), IM_COL32(0,0,0,0));

    const float avH = size.y - 20, avW = size.x - 20;
    const float ar  = (float)m_videoPlayer->GetWidth() / (float)m_videoPlayer->GetHeight();
    float dW = avH * ar, dH = avH;
    if (dW > avW) { dW = avW; dH = dW / ar; }

    ImGui::SetCursorScreenPos(ImVec2(pos.x + (size.x - dW) / 2, pos.y + 10));
    ImGui::Image(m_videoPlayer->GetFrameTexture(), ImVec2(dW, dH), ImVec2(0,0), ImVec2(1,1));
    ImGui::EndChild();
}

void VideoEditState::DrawTimeline(const EditingScreen* parent, const VideoInfo&) {
    const ImVec2 tlAvail = ImGui::GetContentRegionAvail();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
    ImGui::BeginChild("Timeline", ImVec2(tlAvail.x - 20.0f, tlAvail.y), false);
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    ImVec2 cPos  = ImGui::GetCursorScreenPos();
    ImVec2 cSize = ImGui::GetContentRegionAvail();
    dl->AddRectFilled(cPos, ImVec2(cPos.x+cSize.x, cPos.y+cSize.y), IM_COL32(0,0,0,0));

    constexpr float HPAD = 10.0f;
    cPos.x  += HPAD;
    cSize.x -= HPAD * 2.0f;

    // ── Header ──
    DrawTimelineHeader(parent, cPos, ImVec2(cSize.x, HEADER_H));

    // ── Build display tracks ──
    const Config* cfg = CoreServices::Instance().GetConfig();
    struct DisplayTrack {
        std::string     name;
        int             analyzerIdx;
        AudioDeviceType deviceType = AudioDeviceType::Input;
    };
    std::vector<DisplayTrack> displayTracks;

    if (m_audioAnalyzer) {
        const int fileTrackCount = m_audioAnalyzer->GetTrackCount();

        std::vector<std::string>     cfgNames;
        std::vector<AudioDeviceType> cfgTypes;
        if (cfg) {
            for (const auto& t : cfg->nativeAudioTracks) {
                cfgNames.push_back(t.name.empty() ? t.device : t.name);
                cfgTypes.push_back(t.deviceType);
            }
        }

        const int totalRows = std::max(fileTrackCount, (int)cfgNames.size());
        for (int i = 0; i < totalRows; i++) {
            DisplayTrack dt;
            if (i < (int)cfgNames.size() && !cfgNames[i].empty())
                dt.name = cfgNames[i];
            else if (i < fileTrackCount)
                dt.name = m_audioAnalyzer->GetTracks()[i].name;
            else
                dt.name = "Audio Track " + std::to_string(i + 1);

            dt.analyzerIdx = (i < fileTrackCount) ? i : -1;
            dt.deviceType  = (i < (int)cfgTypes.size()) ? cfgTypes[i] : AudioDeviceType::Input;
            displayTracks.push_back(std::move(dt));
        }
    } else {
        if (cfg) {
            for (const auto& t : cfg->nativeAudioTracks) {
                DisplayTrack dt;
                dt.name        = t.name.empty() ? t.device : t.name;
                dt.analyzerIdx = -2;  // loading placeholder
                dt.deviceType  = t.deviceType;
                displayTracks.push_back(std::move(dt));
            }
        }
        if (displayTracks.empty())
            displayTracks.push_back({"Audio", -2, AudioDeviceType::Input});
    }

    // ── Clamp to minimum audio rows ──
    const int audioRows = std::max(MIN_AUDIO, (int)displayTracks.size());

    // ── Layout: tracks are bottom-anchored with BOTTOM_PAD gap ──
    const float totalTracksH = (1 + audioRows) * TRACK_H;
    const float tracksTopY   = cPos.y + cSize.y - totalTracksH - BOTTOM_PAD;

    // ── Clip row (always first / topmost) ──
    const ImVec2 clipPos = ImVec2(cPos.x, tracksTopY);
    const ImVec2 clipSz  = ImVec2(cSize.x, TRACK_H);
    DrawClipTrack(clipPos, clipSz);

    // ── Vertical separator between label column and waveform area ──
    {
        const float sepX = cPos.x + LABEL_W - 2.0f;
        const float top  = tracksTopY;
        const float bot  = tracksTopY + totalTracksH;
        dl->AddLine(ImVec2(sepX, top), ImVec2(sepX, bot),
                    Theme::TL_COL_SEP, 1.5f);
    }

    // ── Audio rows ──
    for (int i = 0; i < audioRows; i++) {
        const ImVec2 tp = ImVec2(cPos.x, tracksTopY + TRACK_H * (i + 1));
        const ImVec2 ts = ImVec2(cSize.x, TRACK_H);

        if (i >= (int)displayTracks.size()) {
            // Empty filler row (to meet MIN_AUDIO)
            dl->AddRectFilled(tp, ImVec2(tp.x+ts.x, tp.y+ts.y), Theme::TL_EMPTY_BG);
            dl->AddRect      (tp, ImVec2(tp.x+ts.x, tp.y+ts.y), Theme::TL_EMPTY_BORDER);
            continue;
        }

        const auto& dt = displayTracks[i];

        if (dt.analyzerIdx == -2) {
            // Loading placeholder
            dl->AddRectFilled(tp, ImVec2(tp.x+ts.x, tp.y+ts.y), BgColor(dt.deviceType));
            dl->AddText(ImVec2(tp.x+8, tp.y + ts.y/2 - 14),
                        WaveColor(dt.deviceType), dt.name.c_str());
            dl->AddText(ImVec2(tp.x+8, tp.y + ts.y/2 + 2),
                        Theme::TL_LABEL_LOADING, "loading...");
            dl->AddRect(tp, ImVec2(tp.x+ts.x, tp.y+ts.y), Theme::TL_EMPTY_BORDER);
        } else {
            DrawTrackBox(tp, ts, dt.name.c_str(), dt.analyzerIdx, dt.deviceType);
        }
    }

    // ── Bracket selection handles — only on Clip track row, drawn as overlay ──
    {
        const float wx       = cPos.x + LABEL_W;
        const float ww       = cSize.x - LABEL_W;
        const float clipTop  = tracksTopY;           // top of Clip row only
        const float clipBot  = tracksTopY + TRACK_H; // bottom of Clip row only
        const float sx       = wx + ww * m_selectStart;
        const float ex       = wx + ww * m_selectEnd;

        constexpr float BAR_W   = 4.0f;
        constexpr float CAP_LEN = 12.0f;
        constexpr float CAP_W   = 4.0f;
        const ImU32 COL_H = Theme::TL_HANDLE;

        auto drawBracket = [&](float hx, bool leftFacing) {
            const float capDir = leftFacing ? 1.0f : -1.0f;
            dl->AddRectFilled(ImVec2(hx - BAR_W*0.5f, clipTop), ImVec2(hx + BAR_W*0.5f, clipBot), COL_H);
            dl->AddRectFilled(ImVec2(hx - BAR_W*0.5f, clipTop), ImVec2(hx - BAR_W*0.5f + capDir*CAP_LEN, clipTop + CAP_W), COL_H);
            dl->AddRectFilled(ImVec2(hx - BAR_W*0.5f, clipBot - CAP_W), ImVec2(hx - BAR_W*0.5f + capDir*CAP_LEN, clipBot), COL_H);
        };

        drawBracket(sx, true);
        drawBracket(ex, false);

        // (InvisibleButtons are registered inside DrawClipTrack BEFORE seek — see there)
    }

    // ── Playhead line — drawn last so it's on top of everything ──
    {
        const float wx  = cPos.x + LABEL_W;
        const float ww  = cSize.x - LABEL_W;
        const float px  = wx + ww * m_playbackProgress;
        const float top = tracksTopY;
        const float bot = tracksTopY + totalTracksH;
        dl->AddLine(ImVec2(px, top), ImVec2(px, bot), Theme::TL_PLAYHEAD, 2.5f);
    }

    ImGui::EndChild();
}

void VideoEditState::DrawClipTrack(ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(pos, ImVec2(pos.x+size.x, pos.y+size.y), Theme::TL_CLIP_BG);
    dl->AddText(ImVec2(pos.x+8, pos.y + size.y/2 - 6),
                Theme::TL_LABEL_CLIP, "Clip");

    const float wx = pos.x + LABEL_W;
    const float ww = size.x - LABEL_W;
    const float cy = pos.y + size.y / 2;
    dl->AddLine(ImVec2(wx, cy), ImVec2(wx+ww, cy), Theme::TL_CLIP_CENTER_LINE, 1);

    const float sx = wx + ww * m_selectStart;
    const float ex = wx + ww * m_selectEnd;

    // Dim outside selection
    if (m_selectStart > 0)
        dl->AddRectFilled(ImVec2(wx, pos.y), ImVec2(sx, pos.y+size.y), Theme::TL_SEL_DIM_CLIP);
    if (m_selectEnd < 1)
        dl->AddRectFilled(ImVec2(ex, pos.y), ImVec2(wx+ww, pos.y+size.y), Theme::TL_SEL_DIM_CLIP);

    dl->AddRect(pos, ImVec2(pos.x+size.x, pos.y+size.y), Theme::TL_CLIP_BORDER);

    // ── IMPORTANT: handle buttons registered FIRST — they win over seek ──
    constexpr float HIT_W = 44.0f;

    ImGui::SetCursorScreenPos(ImVec2(sx - HIT_W*0.5f, pos.y));
    ImGui::InvisibleButton("##SelS", ImVec2(HIT_W, size.y));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        m_selectStart = std::max(0.f, std::min((ImGui::GetMousePos().x - wx) / ww, m_selectEnd - 0.01f));

    ImGui::SetCursorScreenPos(ImVec2(ex - HIT_W*0.5f, pos.y));
    ImGui::InvisibleButton("##SelE", ImVec2(HIT_W, size.y));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        m_selectEnd = std::max(m_selectStart + 0.01f, std::min(1.f, (ImGui::GetMousePos().x - wx) / ww));

    // Seek — registered after handles, so handles take priority when overlapping
    ImGui::SetCursorScreenPos(ImVec2(wx, pos.y));
    ImGui::InvisibleButton("##ClipSeek", ImVec2(ww, size.y));
    if (ImGui::IsItemActivated()) {
        // Drag just started — pause and remember state
        m_wasPlayingBeforeScrub = m_isPlaying;
        if (m_isPlaying) { m_videoPlayer->Pause(); m_isPlaying = false; }
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        const float p = std::clamp((ImGui::GetMousePos().x - wx) / ww, 0.f, 1.f);
        m_videoPlayer->Seek(p * m_videoPlayer->GetDuration());
        m_videoPlayer->Update(0.0f);  // decode frame immediately, don't advance time
    } else if (ImGui::IsItemDeactivated()) {
        // Mouse released — resume if was playing
        if (m_wasPlayingBeforeScrub) { m_videoPlayer->Play(); m_isPlaying = true; }
    } else if (ImGui::IsItemClicked()) {
        const float p = std::clamp((ImGui::GetMousePos().x - wx) / ww, 0.f, 1.f);
        m_videoPlayer->Seek(p * m_videoPlayer->GetDuration());
        m_videoPlayer->Update(0.0f);
    }
}

void VideoEditState::DrawTrackBox(ImVec2 pos, ImVec2 size, const char* label,
                                  const int ti, const AudioDeviceType deviceType) const
{
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const bool noStream = (ti < 0);
    const bool silent   = noStream || (m_audioAnalyzer && m_audioAnalyzer->IsTrackSilent(ti));

    const ImU32 bgCol   = silent ? Theme::TL_SILENT_BG : BgColor(deviceType);
    const ImU32 waveCol = WaveColor(deviceType);

    dl->AddRectFilled(pos, ImVec2(pos.x+size.x, pos.y+size.y), bgCol);

    // Label — wrap if long
    {
        constexpr int MAX_LINE = 16;
        std::string l1, l2;
        SplitLabel(std::string(label), MAX_LINE, l1, l2);

        const ImU32 labelCol = silent
            ? Theme::TL_LABEL_SILENT
            : Theme::TL_LABEL;

        if (l2.empty()) {
            // Single line — vertically centered
            dl->AddText(ImVec2(pos.x+8, pos.y + size.y/2 - 6), labelCol, l1.c_str());
        } else {
            // Two lines
            dl->AddText(ImVec2(pos.x+8, pos.y + size.y/2 - 14), labelCol, l1.c_str());
            dl->AddText(ImVec2(pos.x+8, pos.y + size.y/2 +  2), labelCol, l2.c_str());
        }
    }

    const float wx = pos.x + LABEL_W;
    const float ww = size.x - LABEL_W;
    const float cy = pos.y + size.y / 2;

    if (!noStream && m_audioAnalyzer && !silent && ti < m_audioAnalyzer->GetTrackCount()) {
        const auto& wf  = m_audioAnalyzer->GetWaveform(ti);
        const int   tsec = m_audioAnalyzer->GetTotalSeconds();
        if (tsec > 0) {
            const float pps = ww / static_cast<float>(tsec);
            for (int s = 0; s < static_cast<int>(wf.size()); s++) {
                const float bh = size.y * 0.42f * wf[s];
                const float bx = wx + s * pps;
                dl->AddRectFilled(ImVec2(bx, cy-bh), ImVec2(bx+pps-1, cy),    waveCol);
                dl->AddRectFilled(ImVec2(bx, cy),    ImVec2(bx+pps-1, cy+bh), waveCol);
            }
        }
    } else {
        // Silent or no stream — flat center line only
        dl->AddLine(ImVec2(wx, cy), ImVec2(wx+ww, cy), Theme::TL_CENTER_LINE, 1);
    }

    // Selection dim overlays
    const float sx = wx + ww * m_selectStart;
    const float ex = wx + ww * m_selectEnd;
    if (m_selectStart > 0) dl->AddRectFilled(ImVec2(wx,pos.y), ImVec2(sx,pos.y+size.y), Theme::TL_SEL_DIM);
    if (m_selectEnd   < 1) dl->AddRectFilled(ImVec2(ex,pos.y), ImVec2(wx+ww,pos.y+size.y), Theme::TL_SEL_DIM);

    dl->AddRect(pos, ImVec2(pos.x+size.x, pos.y+size.y), Theme::TL_BORDER);

    // Seek
    if (!noStream) {
        ImGui::SetCursorScreenPos(ImVec2(wx, pos.y));
        ImGui::InvisibleButton(("##TB"+std::to_string(ti)).c_str(), ImVec2(ww, size.y));
        auto doSeek = [&]() {
            float p = std::clamp((ImGui::GetMousePos().x - wx) / ww, 0.f, 1.f);
            m_videoPlayer->Seek(p * m_videoPlayer->GetDuration());
            m_videoPlayer->Update(0.0f);  // decode frame, don't advance time
        };
        if (ImGui::IsItemActivated()) {
            m_wasPlayingBeforeScrub = m_isPlaying;
            if (m_isPlaying) { m_videoPlayer->Pause(); m_isPlaying = false; }
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) { doSeek(); }
        else if (ImGui::IsItemDeactivated()) {
            if (m_wasPlayingBeforeScrub) { m_videoPlayer->Play(); m_isPlaying = true; }
        }
        else if (ImGui::IsItemClicked()) doSeek();
    }
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

void VideoEditState::DrawInfoBar(const EditingScreen* parent, const VideoInfo& video) const {
    const ImGuiViewport* vp    = ImGui::GetMainViewport();
    const float          totalW = vp->WorkSize.x;

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

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("InfoBar", ImVec2(totalW, Theme::TOPBAR_H), false,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();

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
    ImGui::PopStyleColor();
}