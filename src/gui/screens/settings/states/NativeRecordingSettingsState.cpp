#include "gui/screens/settings/states/NativeRecordingSettingsState.h"

#include "gui/Theme.h"
#include "core/CoreServices.h"

#include <cstring>
#include <algorithm>

NativeRecordingSettingsState::NativeRecordingSettingsState() {
    RefreshDeviceLists();
    LoadFromConfig();
}

// ─── Draw ─────────────────────────────────────────────────────────────────────
void NativeRecordingSettingsState::Draw() {
    DrawEncoderSection();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    DrawAudioSection();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    DrawVideoSection();
    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
    DrawReplayBufferSection(180.0f);
}

// ─── Encoder Section ──────────────────────────────────────────────────────────
void NativeRecordingSettingsState::DrawEncoderSection() {
    constexpr float L = 200.0f;

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("ENCODER");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Encoder mode
    ImGui::Text("Encoder");
    ImGui::SameLine(L);
    static const char* encoders[] = { "GPU (VAAPI / NVENC)", "CPU (software)" };
    int enc = (m_encoderMode == EncoderMode::CPU) ? 1 : 0;
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::Combo("##encoder", &enc, encoders, 2)) {
        m_encoderMode = (enc == 1) ? EncoderMode::CPU : EncoderMode::GPU;
        CheckDirty();
    }

    // Fallback
    ImGui::Text("CPU Fallback");
    ImGui::SameLine(L);
    if (ImGui::Checkbox("##fallback", &m_fallbackCpu)) CheckDirty();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("fall back to CPU if GPU encoding fails");
    ImGui::PopStyleColor();

    ImGui::Spacing();

    // Video Codec
    ImGui::Text("Video Codec");
    ImGui::SameLine(L);
    static const char* vcodecs[] = { "h264", "hevc", "av1", "vp8", "vp9", "hevc_hdr", "av1_hdr", "hevc_10bit", "av1_10bit" };
    static constexpr VideoCodec vcodecEnums[] = {
        VideoCodec::H264, VideoCodec::HEVC, VideoCodec::AV1,
        VideoCodec::VP8,  VideoCodec::VP9,
        VideoCodec::HEVC_HDR, VideoCodec::AV1_HDR,
        VideoCodec::HEVC_10BIT, VideoCodec::AV1_10BIT
    };
    int vcIdx = 0;
    for (int i = 0; i < 9; i++) if (vcodecEnums[i] == m_videoCodec) { vcIdx = i; break; }
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::Combo("##vcodec", &vcIdx, vcodecs, 9)) { m_videoCodec = vcodecEnums[vcIdx]; CheckDirty(); }

    // Audio Codec
    ImGui::Text("Audio Codec");
    ImGui::SameLine(L);
    static const char* acodecs[] = { "aac", "opus", "flac" };
    static constexpr AudioCodec acodecEnums[] = { AudioCodec::AAC, AudioCodec::OPUS, AudioCodec::FLAC };
    int acIdx = 0;
    for (int i = 0; i < 3; i++) if (acodecEnums[i] == m_audioCodec) { acIdx = i; break; }
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Combo("##acodec", &acIdx, acodecs, 3)) { m_audioCodec = acodecEnums[acIdx]; CheckDirty(); }
}

// ─── Audio Section ────────────────────────────────────────────────────────────
void NativeRecordingSettingsState::DrawAudioSection() {
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("AUDIO CHANNELS");
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT_HOVER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::ACCENT_ACTIVE);
    if (ImGui::SmallButton("  Refresh  ")) RefreshDeviceLists();
    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    // Audio bitrate
    constexpr float L = 200.0f;
    ImGui::Text("Audio Bitrate (kbps)");
    ImGui::SameLine(L);
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::SliderInt("##abr", &m_audioBitrate, 64, 320)) CheckDirty();

    ImGui::Spacing();

    DrawDeviceColumn("##inputs",  "INPUTS  (Microphone)",  m_inputDevices,  m_inputTracks);
    ImGui::Spacing();
    DrawDeviceColumn("##outputs", "OUTPUTS  (Game / Chat Loopback)", m_outputDevices, m_outputTracks);
}

// ─── Video Section ────────────────────────────────────────────────────────────
void NativeRecordingSettingsState::DrawVideoSection() {
    constexpr float L = 200.0f;

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("VIDEO");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    DrawScreenSelector(L);
    ImGui::Spacing();
    DrawRecordingSettings(L);
}

void NativeRecordingSettingsState::DrawScreenSelector(const float labelW) {
    ImGui::Text("Screen Output");
    ImGui::SameLine(labelW);

    std::string preview = "Select screen...";
    if (!m_screens.empty() && m_selectedScreenIdx < static_cast<int>(m_screens.size())) {
        const ScreenInfo& sel = m_screens[m_selectedScreenIdx];
        preview = sel.output.empty() ? sel.name : sel.output;
        if (sel.width > 0) {
            char buf[64]; snprintf(buf, sizeof(buf), "   %dx%d @%dHz", sel.width, sel.height, sel.refreshRate);
            preview += buf;
        }
    }

    ImGui::SetNextItemWidth(360.0f);
    if (ImGui::BeginCombo("##screen", preview.c_str())) {
        for (int i = 0; i < (int)m_screens.size(); ++i) {
            const ScreenInfo& s = m_screens[i];
            const bool sel      = (m_selectedScreenIdx == i);
            const char* label   = s.output.empty() ? s.name.c_str() : s.output.c_str();
            if (ImGui::Selectable(label, sel)) {
                m_selectedScreenIdx = i;
                std::strncpy(m_screenOutput, label, sizeof(m_screenOutput) - 1);
                CheckDirty();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        Theme::BTN_NEUTRAL);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT_HOVER);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Theme::ACCENT_ACTIVE);
    if (ImGui::SmallButton(" Refresh ")) {
        m_screens = NativeRecorder::GetScreens();
        m_selectedScreenIdx = 0;
        for (int i = 0; i < (int)m_screens.size(); ++i)
            if (m_screens[i].output == std::string(m_screenOutput)) { m_selectedScreenIdx = i; break; }
    }
    ImGui::PopStyleColor(3);
}

void NativeRecordingSettingsState::DrawRecordingSettings(const float labelW) {
    // FPS
    ImGui::Text("FPS");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::BeginCombo("##fps", std::to_string(m_fps).c_str())) {
        for (const int v : { 24, 30, 60, 120, 144, 165 }) {
            const bool sel = (m_fps == v);
            if (ImGui::Selectable(std::to_string(v).c_str(), sel)) { m_fps = v; CheckDirty(); }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Quality preset
    ImGui::Text("Quality");
    ImGui::SameLine(labelW);
    static const char* qualities[] = { "Ultra", "Very High", "High", "Medium", "Low" };
    static constexpr QualityPreset qEnums[] = {
        QualityPreset::Ultra, QualityPreset::VeryHigh, QualityPreset::High,
        QualityPreset::Medium, QualityPreset::Low
    };
    int qIdx = 1;
    for (int i = 0; i < 5; i++) if (qEnums[i] == m_quality) { qIdx = i; break; }
    ImGui::SetNextItemWidth(140.0f);
    if (ImGui::Combo("##quality", &qIdx, qualities, 5)) { m_quality = qEnums[qIdx]; CheckDirty(); }

    // Bitrate mode
    ImGui::Text("Bitrate Mode");
    ImGui::SameLine(labelW);
    static const char* bmodes[] = { "Auto", "QP (Quantization)", "VBR (Variable)", "CBR (Constant)" };
    static constexpr BitrateMode bmEnums[] = { BitrateMode::Auto, BitrateMode::QP, BitrateMode::VBR, BitrateMode::CBR };
    int bmIdx = 0;
    for (int i = 0; i < 4; i++) if (bmEnums[i] == m_bitrateMode) { bmIdx = i; break; }
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::Combo("##bm", &bmIdx, bmodes, 4)) { m_bitrateMode = bmEnums[bmIdx]; CheckDirty(); }

    // Framerate mode
    ImGui::Text("Framerate Mode");
    ImGui::SameLine(labelW);
    static const char* fmodes[]     = { "CFR (Constant)", "VFR (Variable)", "Content" };
    static constexpr FramerateMode fmEnums[] = { FramerateMode::CFR, FramerateMode::VFR, FramerateMode::Content };
    int fmIdx = 1;
    for (int i = 0; i < 3; i++) if (fmEnums[i] == m_framerateMode) { fmIdx = i; break; }
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::Combo("##fm", &fmIdx, fmodes, 3)) { m_framerateMode = fmEnums[fmIdx]; CheckDirty(); }

    // Color range
    ImGui::Text("Color Range");
    ImGui::SameLine(labelW);
    static const char* cranges[] = { "Limited (TV)", "Full (PC)" };
    int crIdx = (m_colorRange == ColorRange::Full) ? 1 : 0;
    ImGui::SetNextItemWidth(140.0f);
    if (ImGui::Combo("##cr", &crIdx, cranges, 2)) { m_colorRange = (crIdx == 1) ? ColorRange::Full : ColorRange::Limited; CheckDirty(); }

    // Tune
    ImGui::Text("Tune");
    ImGui::SameLine(labelW);
    static const char* tunes[]         = { "Quality", "Performance" };
    static constexpr TuneProfile tuneEnums[] = { TuneProfile::Quality, TuneProfile::Performance };
    int tIdx = 0;
    for (int i = 0; i < 2; i++) if (tuneEnums[i] == m_tune) { tIdx = i; break; }
    ImGui::SetNextItemWidth(140.0f);
    if (ImGui::Combo("##tune", &tIdx, tunes, 2)) { m_tune = tuneEnums[tIdx]; CheckDirty(); }

    // Container
    ImGui::Text("Container");
    ImGui::SameLine(labelW);
    static const char* containers[]        = { "MP4", "MKV", "FLV" };
    static constexpr ContainerFormat cfEnums[] = { ContainerFormat::MP4, ContainerFormat::MKV, ContainerFormat::FLV };
    int cfIdx = 0;
    for (int i = 0; i < 3; i++) if (cfEnums[i] == m_containerFmt) { cfIdx = i; break; }
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Combo("##cf", &cfIdx, containers, 3)) { m_containerFmt = cfEnums[cfIdx]; CheckDirty(); }

    // Cursor
    ImGui::Text("Show Cursor");
    ImGui::SameLine(labelW);
    if (ImGui::Checkbox("##cursor", &m_showCursor)) CheckDirty();
}

// ─── Replay Buffer Section ────────────────────────────────────────────────────
void NativeRecordingSettingsState::DrawReplayBufferSection(const float L) {
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextUnformatted("REPLAY BUFFER");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
    ImGui::TextWrapped("gpu-screen-recorder continuously keeps the last N seconds in memory. "
        "Press Save Clip to instantly write that window to a file.");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Duration
    struct DurOpt { int s; const char* l; };
    static const DurOpt opts[] = {
        {30,"30 seconds"},{60,"1 minute"},{120,"2 minutes"},
        {180,"3 minutes"},{240,"4 minutes"},{300,"5 minutes"}
    };
    const char* curLabel = "30 seconds";
    for (const auto& o : opts) if (o.s == m_replayDuration) { curLabel = o.l; break; }

    ImGui::Text("Buffer Duration");
    ImGui::SameLine(L);
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::BeginCombo("##dur", curLabel)) {
        for (const auto&[s, l] : opts) {
            const bool sel = (m_replayDuration == s);
            if (ImGui::Selectable(l, sel)) { m_replayDuration = s; CheckDirty(); }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Storage
    ImGui::Text("Buffer Storage");
    ImGui::SameLine(L);
    static const char* storages[] = { "RAM (faster, uses memory)", "Disk (slower, saves memory)" };
    int sIdx = (m_replayStorage == ReplayStorage::Disk) ? 1 : 0;
    ImGui::SetNextItemWidth(260.0f);
    if (ImGui::Combo("##storage", &sIdx, storages, 2)) {
        m_replayStorage = (sIdx == 1) ? ReplayStorage::Disk : ReplayStorage::RAM;
        CheckDirty();
    }
}

// ─── Device column ────────────────────────────────────────────────────────────
void NativeRecordingSettingsState::DrawDeviceColumn(
    const char* id, const char* label,
    const std::vector<AudioDevice>& devs,
    std::vector<SelectedTrack>& tracks)
{
    const float colW = ImGui::GetContentRegionAvail().x;
    constexpr float boxH = 160.0f;

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_PRIMARY);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::BG_DARK);
    ImGui::BeginChild(id, ImVec2(colW, boxH), true);

    int removeIdx = -1;
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
        ImGui::PushID(i);
        const SelectedTrack& t = tracks[i];
        ImGui::Text("  %s", t.customName.empty() ? t.deviceId.c_str() : t.customName.c_str());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        Theme::DANGER);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f,0.2f,0.2f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.5f,0.1f,0.1f,1.0f));
        if (ImGui::SmallButton("x")) removeIdx = i;
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }
    if (removeIdx >= 0) { tracks.erase(tracks.begin() + removeIdx); CheckDirty(); }

    if (tracks.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_MUTED);
        ImGui::TextUnformatted("  No devices added");
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SetNextItemWidth(colW);
    if (ImGui::BeginCombo((std::string("##add") + id).c_str(), "  + Add Device")) {
        for (int i = 0; i < static_cast<int>(devs.size()); ++i) {
            ImGui::PushID(i);
            if (ImGui::Selectable(devs[i].displayName.c_str())) {
                SelectedTrack t;
                t.customName = devs[i].displayName;
                t.deviceId   = devs[i].id;
                t.deviceType = devs[i].type;
                tracks.push_back(std::move(t));
                CheckDirty();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
}

void NativeRecordingSettingsState::RefreshDeviceLists() {
    m_inputDevices  = AudioDeviceEnumerator::GetInputDevices();
    m_outputDevices = AudioDeviceEnumerator::GetOutputDevices();
    m_screens       = NativeRecorder::GetScreens();
}

void NativeRecordingSettingsState::LoadFromConfig() {
    const auto* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    m_videoCodec    = cfg->nativeVideoCodec;
    m_audioCodec    = cfg->nativeAudioCodec;
    m_encoderMode   = cfg->nativeEncoder;
    m_fallbackCpu   = cfg->nativeFallbackCpu;
    m_quality       = cfg->nativeQuality;
    m_bitrateMode   = cfg->nativeBitrateMode;
    m_videoBitrate  = cfg->nativeVideoBitrate;
    m_audioBitrate  = cfg->nativeAudioBitrate;
    m_fps           = cfg->nativeFPS;
    m_replayDuration = cfg->nativeClipDuration;
    m_replayStorage = cfg->nativeReplayStorage;
    m_showCursor    = cfg->nativeShowCursor;
    m_containerFmt  = cfg->nativeContainerFormat;
    m_colorRange    = cfg->nativeColorRange;
    m_framerateMode = cfg->nativeFramerateMode;
    m_tune          = cfg->nativeTune;
    m_audioMode     = cfg->nativeAudioMode;

    m_inputTracks  = TrackListFromConfig(cfg->nativeAudioTracks, AudioDeviceType::Input);
    m_outputTracks = TrackListFromConfig(cfg->nativeAudioTracks, AudioDeviceType::Output);

    std::strncpy(m_screenOutput, cfg->nativeScreenOutput.c_str(), sizeof(m_screenOutput) - 1);
    m_selectedScreenIdx = 0;
    for (int i = 0; i < static_cast<int>(m_screens.size()); ++i) {
        if (m_screens[i].output == cfg->nativeScreenOutput) {
            m_selectedScreenIdx = i; break;
        }
    }

    SyncOriginals();
    m_dirty = false;
}

// ─── Save ─────────────────────────────────────────────────────────────────────
void NativeRecordingSettingsState::SaveChanges() const {
    auto* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    cfg->nativeVideoCodec    = m_videoCodec;
    cfg->nativeAudioCodec    = m_audioCodec;
    cfg->nativeEncoder       = m_encoderMode;
    cfg->nativeFallbackCpu   = m_fallbackCpu;
    cfg->nativeQuality       = m_quality;
    cfg->nativeBitrateMode   = m_bitrateMode;
    cfg->nativeVideoBitrate  = m_videoBitrate;
    cfg->nativeAudioBitrate  = m_audioBitrate;
    cfg->nativeFPS           = m_fps;
    cfg->nativeClipDuration  = m_replayDuration;
    cfg->nativeReplayStorage = m_replayStorage;
    cfg->nativeShowCursor    = m_showCursor;
    cfg->nativeContainerFormat = m_containerFmt;
    cfg->nativeColorRange    = m_colorRange;
    cfg->nativeFramerateMode = m_framerateMode;
    cfg->nativeTune          = m_tune;
    cfg->nativeAudioMode     = m_audioMode;
    cfg->nativeScreenOutput  = m_screenOutput;
    cfg->nativeAudioTracks   = TrackListToConfig(m_inputTracks, m_outputTracks);

    if (!cfg->Save()) std::cerr << "[Settings] Config save failed\n";
    CoreServices::Instance().GetRecordingManager()->ApplyConfig();

    auto* mutableThis = const_cast<NativeRecordingSettingsState*>(this);
    mutableThis->SyncOriginals();
    mutableThis->CheckDirty();
}

// ─── Dirty tracking ───────────────────────────────────────────────────────────
void NativeRecordingSettingsState::SyncOriginals() {
    m_origVideoCodec       = m_videoCodec;
    m_origAudioCodec       = m_audioCodec;
    m_origEncoderMode      = m_encoderMode;
    m_origFallbackCpu      = m_fallbackCpu;
    m_origQuality          = m_quality;
    m_origBitrateMode      = m_bitrateMode;
    m_origVideoBitrate     = m_videoBitrate;
    m_origAudioBitrate     = m_audioBitrate;
    m_origFps              = m_fps;
    m_origReplayDuration   = m_replayDuration;
    m_origReplayStorage    = m_replayStorage;
    m_origShowCursor       = m_showCursor;
    m_origContainerFmt     = m_containerFmt;
    m_origColorRange       = m_colorRange;
    m_origFramerateMode    = m_framerateMode;
    m_origTune             = m_tune;
    m_origAudioMode        = m_audioMode;
    m_origSelectedScreenIdx = m_selectedScreenIdx;
    m_origInputTracks      = m_inputTracks;
    m_origOutputTracks     = m_outputTracks;
    std::strncpy(m_origScreenOutput, m_screenOutput, sizeof(m_origScreenOutput) - 1);
}

void NativeRecordingSettingsState::CheckDirty() {
    m_dirty =
            m_videoCodec    != m_origVideoCodec    ||
            m_audioCodec    != m_origAudioCodec    ||
            m_encoderMode   != m_origEncoderMode   ||
            m_fallbackCpu   != m_origFallbackCpu   ||
            m_quality       != m_origQuality       ||
            m_bitrateMode   != m_origBitrateMode   ||
            m_videoBitrate  != m_origVideoBitrate  ||
            m_audioBitrate  != m_origAudioBitrate  ||
            m_fps           != m_origFps           ||
            m_replayDuration != m_origReplayDuration ||
            m_replayStorage != m_origReplayStorage ||
            m_showCursor    != m_origShowCursor    ||
            m_containerFmt  != m_origContainerFmt  ||
            m_colorRange    != m_origColorRange    ||
            m_framerateMode != m_origFramerateMode ||
            m_tune          != m_origTune          ||
            m_audioMode     != m_origAudioMode     ||
            m_selectedScreenIdx != m_origSelectedScreenIdx ||
            m_inputTracks   != m_origInputTracks   ||
            m_outputTracks  != m_origOutputTracks  ||
            std::strcmp(m_screenOutput, m_origScreenOutput) != 0;
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
std::vector<SelectedTrack> NativeRecordingSettingsState::TrackListFromConfig(
    const std::vector<AudioTrack>& src, const AudioDeviceType type)
{
    std::vector<SelectedTrack> out;
    for (const auto&[name, device, deviceType] : src) {
        if (deviceType != type) continue;
        SelectedTrack s;
        s.customName = name;
        s.deviceId   = device;
        s.deviceType = deviceType;
        out.push_back(std::move(s));
    }
    return out;
}

std::vector<AudioTrack> NativeRecordingSettingsState::TrackListToConfig(
    const std::vector<SelectedTrack> &in, const std::vector<SelectedTrack> &out) {
    std::vector<AudioTrack> all;
    for (const auto& s : in)  { AudioTrack t; t.name=s.customName; t.device=s.deviceId; t.deviceType=AudioDeviceType::Input;  all.push_back(t); }
    for (const auto& s : out) { AudioTrack t; t.name=s.customName; t.device=s.deviceId; t.deviceType=AudioDeviceType::Output; all.push_back(t); }
    return all;
}