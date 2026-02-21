#include "gui/screens/settings/states/NativeRecordingSettingsState.h"

#include <cstring>
#include <algorithm>

NativeRecordingSettingsState::NativeRecordingSettingsState() {
    RefreshDeviceLists();
    LoadFromConfig();
}

void NativeRecordingSettingsState::RefreshDeviceLists() {
    m_inputDevices  = AudioDeviceEnumerator::GetInputDevices();
    m_outputDevices = AudioDeviceEnumerator::GetOutputDevices();
    m_screens       = NativeRecorder::GetScreens();
}
void NativeRecordingSettingsState::LoadFromConfig() {
    const auto* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    switch (cfg->nativeAudioMode) {
        case AudioMode::Separated: m_audioModeIndex = 1; break;
        case AudioMode::Virtual:   m_audioModeIndex = 2; break;
        default:                   m_audioModeIndex = 0; break;
    }

    m_inputTracks  = TrackListFromConfig(cfg->nativeAudioTracks, AudioDeviceType::Input);
    m_outputTracks = TrackListFromConfig(cfg->nativeAudioTracks, AudioDeviceType::Output);

    std::strncpy(m_screenOutput, cfg->nativeScreenOutput.c_str(), sizeof(m_screenOutput) - 1);

    m_selectedScreenIdx = 0;
    for (int i = 0; i < static_cast<int>(m_screens.size()); ++i) {
        if (m_screens[i].output == cfg->nativeScreenOutput) {
            m_selectedScreenIdx = i;
            break;
        }
    }

    std::strncpy(m_videoCodec, cfg->nativeVideoCodec.c_str(), sizeof(m_videoCodec) - 1);
    std::strncpy(m_audioCodec, cfg->nativeAudioCodec.c_str(), sizeof(m_audioCodec) - 1);
    m_videoBitrate   = cfg->nativeVideoBitrate;
    m_audioBitrate   = cfg->nativeAudioBitrate;
    m_fps            = cfg->nativeFPS;
    m_replayDuration = cfg->clipDuration;

    SyncOriginals();
    m_dirty = false;
}

// ─── Dirty tracking ───────────────────────────────────────────────────────
void NativeRecordingSettingsState::SyncOriginals() {
    m_origAudioModeIndex    = m_audioModeIndex;
    m_origSelectedScreenIdx = m_selectedScreenIdx;
    m_origVideoBitrate      = m_videoBitrate;
    m_origAudioBitrate      = m_audioBitrate;
    m_origFps               = m_fps;
    m_origReplayDuration    = m_replayDuration;
    m_origInputTracks       = m_inputTracks;
    m_origOutputTracks      = m_outputTracks;
    // char arrays
    std::strncpy(m_origScreenOutput, m_screenOutput, sizeof(m_origScreenOutput) - 1);
    std::strncpy(m_origVideoCodec,   m_videoCodec,   sizeof(m_origVideoCodec)   - 1);
    std::strncpy(m_origAudioCodec,   m_audioCodec,   sizeof(m_origAudioCodec)   - 1);
}

void NativeRecordingSettingsState::CheckDirty() {
    m_dirty = (m_audioModeIndex    != m_origAudioModeIndex)
           || (m_selectedScreenIdx != m_origSelectedScreenIdx)
           || (m_videoBitrate      != m_origVideoBitrate)
           || (m_audioBitrate      != m_origAudioBitrate)
           || (m_fps               != m_origFps)
           || (m_replayDuration    != m_origReplayDuration)
           || (m_inputTracks       != m_origInputTracks)
           || (m_outputTracks      != m_origOutputTracks)
           || (std::strcmp(m_screenOutput, m_origScreenOutput) != 0)
           || (std::strcmp(m_videoCodec,   m_origVideoCodec)   != 0)
           || (std::strcmp(m_audioCodec,   m_origAudioCodec)   != 0);
}

// ─── Conversion helpers ────────────────────────────────────────────────────
std::vector<SelectedTrack> NativeRecordingSettingsState::TrackListFromConfig(
    const std::vector<AudioTrack>& src, AudioDeviceType type)
{
    std::vector<SelectedTrack> out;
    for (const auto& t : src) {
        if (t.deviceType != type) continue;
        SelectedTrack s;
        s.customName = t.name;
        s.deviceId   = t.device;
        s.deviceType = t.deviceType;
        out.push_back(std::move(s));
    }
    return out;
}

std::vector<AudioTrack> NativeRecordingSettingsState::TrackListToConfig(
    const std::vector<SelectedTrack> &in,
    const std::vector<SelectedTrack> &out) {
    std::vector<AudioTrack> all;
    for (const auto& s : in) {
        AudioTrack t;
        t.name       = s.customName;
        t.device     = s.deviceId;
        t.deviceType = AudioDeviceType::Input;
        all.push_back(std::move(t));
    }
    for (const auto& s : out) {
        AudioTrack t;
        t.name       = s.customName;
        t.device     = s.deviceId;
        t.deviceType = AudioDeviceType::Output;
        all.push_back(std::move(t));
    }
    return all;
}

// ─── Draw ──────────────────────────────────────────────────────────────────
void NativeRecordingSettingsState::Draw() {
    DrawAudioSection();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    DrawVideoSection();
}

// ─── Audio Section ──────────────────────────────────────────────────────────
void NativeRecordingSettingsState::DrawAudioSection() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.60f,1.0f));
    ImGui::TextUnformatted("AUDIO MODE");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    const char* modes[] = { "Mixed", "Separated", "Virtual" };
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::Combo("##audio_mode", &m_audioModeIndex, modes, 3)) CheckDirty();

    ImGui::SameLine(0, 16.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.55f,1.0f));
    switch (m_audioModeIndex) {
        case 0: ImGui::TextUnformatted("All channels mixed into a single track"); break;
        case 1: ImGui::TextUnformatted("Each channel is recorded as a separate track."); break;
        case 2: ImGui::TextUnformatted("Routing is performed via the virtual microphone"); break;
        default: ;
    }
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.60f,1.0f));
    ImGui::TextUnformatted("AUDIO CHANNELS");
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f,0.18f,0.22f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f,0.24f,0.30f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.14f,0.14f,0.18f,1.0f));
    if (ImGui::SmallButton("  Refresh Devices  ")) RefreshDeviceLists();
    ImGui::PopStyleColor(3);

    ImGui::Spacing();

    const bool tracksEnabled = (m_audioModeIndex == 1);
    ImGui::BeginDisabled(!tracksEnabled);
    DrawDeviceColumn("##inputs",  "INPUTS  (Capture)",
                     m_inputDevices, m_inputTracks);
    ImGui::Spacing();
    DrawDeviceColumn("##outputs", "OUTPUTS  (Loopback)",
                     m_outputDevices, m_outputTracks);
    ImGui::EndDisabled();
}

// ─── Device column ──────────────────────────────────────────────────────────

void NativeRecordingSettingsState::DrawDeviceColumn(
    const char *id,
    const char *label,
    const std::vector<AudioDevice> &devs,
    std::vector<SelectedTrack> &tracks)
{
    const float colW = ImGui::GetContentRegionAvail().x;
    constexpr float boxH = 180.0f;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f,0.75f,0.80f,1.0f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f,0.09f,0.11f,1.0f));
    ImGui::BeginChild(id, ImVec2(colW, boxH), true);

    int removeIdx = -1;
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
        const SelectedTrack& t = tracks[i];
        ImGui::PushID(i);

        const bool isDefault = (t.deviceId == "default");
        if (isDefault) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f,0.9f,1.0f,1.0f));
        ImGui::Text("  %s", t.customName.empty() ? t.deviceId.c_str() : t.customName.c_str());
        if (isDefault) ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f,0.2f,0.2f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f,0.2f,0.2f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.5f,0.1f,0.1f,1.0f));
        if (ImGui::SmallButton("x")) removeIdx = i;
        ImGui::PopStyleColor(3);

        ImGui::PopID();
    }

    if (removeIdx >= 0) {
        tracks.erase(tracks.begin() + removeIdx);
        CheckDirty();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SetNextItemWidth(colW);
    if (ImGui::BeginCombo((std::string("##add") + id).c_str(), "  + Add Device")) {
        for (int i = 0; i < static_cast<int>(devs.size()); ++i) {
            const AudioDevice& dev = devs[i];
            ImGui::PushID(i);
            if (ImGui::Selectable(dev.displayName.c_str())) {
                SelectedTrack t;
                t.customName = dev.displayName;
                t.deviceId   = dev.id;
                t.deviceType = dev.type;
                tracks.push_back(std::move(t));
                CheckDirty();
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
}

// ─── Video Section ──────────────────────────────────────────────────────────
void NativeRecordingSettingsState::DrawVideoSection() {
    constexpr float labelW = 180.0f;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.60f,1.0f));
    ImGui::TextUnformatted("VIDEO");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Text("Screen Output");
    ImGui::SameLine(labelW);

    std::string previewStr = "Select screen...";
    if (!m_screens.empty() && m_selectedScreenIdx < static_cast<int>(m_screens.size())) {
        const ScreenInfo& sel = m_screens[m_selectedScreenIdx];
        char buf[128];
        snprintf(buf, sizeof(buf), "%s   %dx%d @%dHz",
                 sel.output.empty() ? sel.name.c_str() : sel.output.c_str(),
                 sel.width, sel.height, sel.refreshRate);
        previewStr = buf;
    }

    ImGui::SetNextItemWidth(360.0f);
    if (ImGui::BeginCombo("##screen_out", previewStr.c_str())) {
        for (int i = 0; i < static_cast<int>(m_screens.size()); ++i) {
            const ScreenInfo& s = m_screens[i];
            const bool selected = (m_selectedScreenIdx == i);

            char lineBuf[128];
            snprintf(lineBuf, sizeof(lineBuf), "%s",
                     s.output.empty() ? s.name.c_str() : s.output.c_str());
            char resBuf[64];
            snprintf(resBuf, sizeof(resBuf), " %dx%d @%dHz", s.width, s.height, s.refreshRate);

            if (ImGui::Selectable(lineBuf, selected)) {
                m_selectedScreenIdx = i;
                std::strncpy(m_screenOutput,
                             s.output.empty() ? s.name.c_str() : s.output.c_str(),
                             sizeof(m_screenOutput) - 1);
                CheckDirty();
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f,0.45f,0.50f,1.0f));
            ImGui::TextUnformatted(resBuf);
            ImGui::PopStyleColor();
            if (selected) ImGui::SetItemDefaultFocus();
        }
        if (m_screens.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.55f,1.0f));
            ImGui::TextUnformatted("  Screen not found");
            ImGui::PopStyleColor();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f,0.18f,0.22f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f,0.24f,0.30f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.14f,0.14f,0.18f,1.0f));
    if (ImGui::SmallButton(" Renew ")) {
        m_screens = NativeRecorder::GetScreens();
        m_selectedScreenIdx = 0;
        for (int i = 0; i < static_cast<int>(m_screens.size()); ++i) {
            if (m_screens[i].output == std::string(m_screenOutput)) {
                m_selectedScreenIdx = i;
                break;
            }
        }
    }
    ImGui::PopStyleColor(3);

    ImGui::Text("Video Codec");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::InputText("##vid_codec", m_videoCodec, sizeof(m_videoCodec))) CheckDirty();

    ImGui::Text("Audio Codec");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::InputText("##aud_codec", m_audioCodec, sizeof(m_audioCodec))) CheckDirty();

    ImGui::Text("Video Bitrate (kbps)");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::SliderInt("##vid_br", &m_videoBitrate, 1000, 50000)) CheckDirty();

    ImGui::Text("Audio Bitrate (kbps)");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::SliderInt("##aud_br", &m_audioBitrate, 64, 320)) CheckDirty();

    ImGui::Text("FPS");
    ImGui::SameLine(labelW);
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::BeginCombo("##fps", std::to_string(m_fps).c_str())) {
        for (constexpr int fpsOptions[] = { 30, 60, 120 }; const int v : fpsOptions) {
            const bool sel = (m_fps == v);
            if (ImGui::Selectable(std::to_string(v).c_str(), sel)) {
                m_fps = v;
                CheckDirty();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.55f,0.60f,1.0f));
    ImGui::TextUnformatted("REPLAY BUFFER");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f,0.5f,0.55f,1.0f));
    ImGui::TextWrapped("When the replay buffer is enabled, the selected duration is stored in memory. "
                       "When the \"Save Clip\" button is pressed, the current buffer is written to the file.");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Text("Buffer Time");
    ImGui::SameLine(labelW);

    struct DurationOption { int seconds; const char* label; };
    static const DurationOption durationOpts[] = {
        { 30,  "30 seconds" }, { 60,  "1 minute"  }, { 120, "2 minute"  },
        { 180, "3 minute"  }, { 240, "4 minute"  }, { 300, "5 minute"  },
    };
    static constexpr int durationOptCount = 6;

    auto currentLabel = "30 saniye";
    for (const auto&[seconds, label] : durationOpts)
        if (seconds == m_replayDuration) { currentLabel = label; break; }

    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::BeginCombo("##replay_dur", currentLabel)) {
        for (int i = 0; i < durationOptCount; ++i) {
            const bool sel = (m_replayDuration == durationOpts[i].seconds);
            if (ImGui::Selectable(durationOpts[i].label, sel)) {
                m_replayDuration = durationOpts[i].seconds;
                CheckDirty();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine(0, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f,0.45f,0.50f,1.0f));
    char marginInfo[48];
    snprintf(marginInfo, sizeof(marginInfo), "(is stored in memory as %ds)", m_replayDuration + 5);
    ImGui::TextUnformatted(marginInfo);
    ImGui::PopStyleColor();
}

// ─── Save ──────────────────────────────────────────────────────────────────────
void NativeRecordingSettingsState::SaveChanges() const {
    auto* cfg = CoreServices::Instance().GetConfig();
    if (!cfg) return;

    switch (m_audioModeIndex) {
        case 1:  cfg->nativeAudioMode = AudioMode::Separated; break;
        case 2:  cfg->nativeAudioMode = AudioMode::Virtual;   break;
        default: cfg->nativeAudioMode = AudioMode::Mixed;     break;
    }

    cfg->nativeAudioTracks  = TrackListToConfig(m_inputTracks, m_outputTracks);
    cfg->nativeScreenOutput = m_screenOutput;
    cfg->nativeVideoCodec   = m_videoCodec;
    cfg->nativeAudioCodec   = m_audioCodec;
    cfg->nativeVideoBitrate = m_videoBitrate;
    cfg->nativeAudioBitrate = m_audioBitrate;
    cfg->nativeFPS          = m_fps;
    cfg->clipDuration       = m_replayDuration;

    if (!cfg->Save()) {
        std::cerr << "[Settings/RecordingSettingsState] Config save failed" << std::endl;
    } else {
        std::cout << "[Settings/RecordingSettingsState] Config updated successfully" << std::endl;
    }
}