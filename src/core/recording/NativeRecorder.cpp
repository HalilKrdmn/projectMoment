#include "core/recording/NativeRecorder.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>
#include <thread>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

// ──────────────────────────────────────────────────────────────────────────
static const char* ToGSR(const VideoCodec v) {
    switch (v) {
        case VideoCodec::HEVC:      return "hevc";
        case VideoCodec::AV1:       return "av1";
        case VideoCodec::VP8:       return "vp8";
        case VideoCodec::VP9:       return "vp9";
        case VideoCodec::HEVC_HDR:  return "hevc_hdr";
        case VideoCodec::AV1_HDR:   return "av1_hdr";
        case VideoCodec::HEVC_10BIT:return "hevc_10bit";
        case VideoCodec::AV1_10BIT: return "av1_10bit";
        default:                    return "h264";
    }
}
static const char* ToGSR(const AudioCodec v) {
    switch (v) {
        case AudioCodec::OPUS: return "opus";
        case AudioCodec::FLAC: return "flac";
        default:               return "aac";
    }
}
static const char* ToGSR(const EncoderMode v)     { return (v == EncoderMode::CPU)         ? "cpu"         : "gpu"; }
static const char* ToGSR(const ReplayStorage v)   { return (v == ReplayStorage::Disk)       ? "disk"        : "ram"; }
static const char* ToGSR(const ColorRange v)      { return (v == ColorRange::Full)          ? "full"        : "limited"; }
static const char* ToGSR(const TuneProfile v)     { return (v == TuneProfile::Performance)  ? "performance" : "quality"; }
static const char* ToGSR(const ContainerFormat v) {
    switch (v) {
        case ContainerFormat::MKV: return "mkv";
        case ContainerFormat::FLV: return "flv";
        default:                   return "mp4";
    }
}
static const char* ToGSR(const FramerateMode v) {
    switch (v) {
        case FramerateMode::VFR:     return "vfr";
        case FramerateMode::Content: return "content";
        default:                     return "cfr";
    }
}
static const char* ToGSR(const BitrateMode v) {
    switch (v) {
        case BitrateMode::QP:  return "qp";
        case BitrateMode::VBR: return "vbr";
        case BitrateMode::CBR: return "cbr";
        default:               return "auto";
    }
}
static const char* ToGSR(const QualityPreset v) {
    switch (v) {
        case QualityPreset::Ultra:    return "ultra";
        case QualityPreset::VeryHigh: return "very_high";
        case QualityPreset::High:     return "high";
        case QualityPreset::Medium:   return "medium";
        case QualityPreset::Low:      return "low";
        default:                      return "very_high";
    }
}
// ──────────────────────────────────────────────────────────────────────────

NativeRecorder::NativeRecorder() {
    if (const char* home = std::getenv("HOME"))
        m_outputDir = fs::path(home) / "Videos";
    CheckDependencies();
}

NativeRecorder::~NativeRecorder() {
    StopRecording();
}

// ── Configuration ────────────────────────────────────────────────────────
void NativeRecorder::SetAudioTracks(const std::vector<AudioTrack>& t)  { m_audioTracks   = t; }
void NativeRecorder::SetScreen(const std::string& o)                   { m_screenOutput  = o; }
void NativeRecorder::SetVideoCodec(VideoCodec c)                       { m_videoCodec    = c; }
void NativeRecorder::SetAudioCodec(AudioCodec c)                       { m_audioCodec    = c; }
void NativeRecorder::SetEncoder(EncoderMode e)                         { m_encoder       = e; }
void NativeRecorder::SetFallbackCpu(bool f)                            { m_fallbackCpu   = f; }
void NativeRecorder::SetQuality(QualityPreset q)                       { m_quality       = q; }
void NativeRecorder::SetBitrateMode(BitrateMode m)                     { m_bitrateMode   = m; }
void NativeRecorder::SetVideoBitrate(int kbps)                         { m_videoBitrate  = kbps; }
void NativeRecorder::SetAudioBitrate(int kbps)                         { m_audioBitrate  = kbps; }
void NativeRecorder::SetFPS(int fps)                                   { m_fps           = fps; }
void NativeRecorder::SetClipDuration(int s)                            { m_clipDuration  = s; }
void NativeRecorder::SetReplayStorage(ReplayStorage s)                 { m_replayStorage = s; }
void NativeRecorder::SetShowCursor(bool show)                          { m_showCursor    = show; }
void NativeRecorder::SetContainerFormat(ContainerFormat f)             { m_containerFormat = f; }
void NativeRecorder::SetColorRange(ColorRange cr)                      { m_colorRange    = cr; }
void NativeRecorder::SetFramerateMode(FramerateMode fm)                { m_framerateMode = fm; }
void NativeRecorder::SetTune(TuneProfile t)                            { m_tune          = t; }
void NativeRecorder::SetOutputDirectory(const fs::path& dir) {
    m_outputDir = dir;
    fs::create_directories(dir);
}

// ─── Command builder ──────────────────────────────────────────────────────
std::string NativeRecorder::BuildCommand() const {
    std::ostringstream cmd;
    cmd << "gpu-screen-recorder";

    // Capture target
    if (m_screenOutput.empty() || m_screenOutput == "AUTO") {
        cmd << " -w screen";
    } else {
        std::string cleanOutput = m_screenOutput;
        if (const size_t pipePos = cleanOutput.find('|'); pipePos != std::string::npos) {
            cleanOutput = cleanOutput.substr(0, pipePos);
        }
        cmd << " -w \"" << cleanOutput << "\"";
    }

    // Video
    cmd << " -f "    << m_fps;
    cmd << " -k "    << ToGSR(m_videoCodec);
    cmd << " -q "    << ToGSR(m_quality);
    cmd << " -bm "   << ToGSR(m_bitrateMode);
    cmd << " -fm "   << ToGSR(m_framerateMode);
    cmd << " -cr "   << ToGSR(m_colorRange);
    cmd << " -tune " << ToGSR(m_tune);

    // Container
    cmd << " -c "    << ToGSR(m_containerFormat);

    // Encoder
    cmd << " -encoder " << ToGSR(m_encoder);
    cmd << " -fallback-cpu-encoding " << (m_fallbackCpu ? "yes" : "no");

    // Audio
    // Build list of valid audio sources first
    std::vector<std::string> audioSources;
    for (const auto& t : m_audioTracks) {
        if (t.device.empty()) continue;
        if (t.deviceType == AudioDeviceType::Output)
            audioSources.push_back(t.device + ".monitor");
        else
            audioSources.push_back(t.device);
    }
    // Only add audio flags if there are actual sources — GSR rejects -ac/-ab without -a
    if (!audioSources.empty()) {
        cmd << " -ac " << ToGSR(m_audioCodec);
        cmd << " -ab " << m_audioBitrate;
        for (const auto& src : audioSources)
            cmd << " -a \"" << src << "\"";
    }

    // Replay buffer
    cmd << " -r "               << m_clipDuration;
    cmd << " -replay-storage "  << ToGSR(m_replayStorage);

    // Misc
    cmd << " -cursor " << (m_showCursor ? "yes" : "no");

    // Output directory
    cmd << " -ro \"" << m_outputDir.string() << "\"";
    cmd << " -o \"" << m_outputDir.string() << "\"";

    return cmd.str();
}


// ── Recording control ──────────────────────────────────────────────────
bool NativeRecorder::StartRecording() {
    if (m_recording) { printf("[NativeRecorder] Already recording\n"); return true; }

    const std::string cmd = BuildCommand();
    printf("[NativeRecorder] Start: %s\n", cmd.c_str());

    m_gsrPid = fork();
    if (m_gsrPid == 0) {
        // Redirect stderr to suppress fps-update spam; stdout kept for errors
        if (const int dn = open("/dev/null", O_WRONLY); dn >= 0) {
            dup2(dn, STDERR_FILENO); close(dn);
        }
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(1);
    }
    if (m_gsrPid < 0) {
        fprintf(stderr, "[NativeRecorder] fork() failed\n");
        m_gsrPid = -1;
        return false;
    }

    // Wait a moment, then verify it's still alive
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    int status;
    if (waitpid(m_gsrPid, &status, WNOHANG) > 0) {
        fprintf(stderr, "[NativeRecorder] GSR exited immediately — check codec/screen/device\n");
        m_gsrPid = -1;
        return false;
    }

    m_recording = true;
    printf("[NativeRecorder] Recording started (pid=%d, buffer=%ds)\n", m_gsrPid, m_clipDuration);
    UpdateStatus("Recording...");
    return true;
}

void NativeRecorder::StopRecording() {
    if (!m_recording && m_gsrPid <= 0) return;
    m_recording = false;

    if (m_gsrPid > 0) {
        kill(m_gsrPid, SIGINT);
        int status, waited = 0;
        while (waited++ < 50) {
            if (waitpid(m_gsrPid, &status, WNOHANG) > 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (waited >= 50) { kill(m_gsrPid, SIGKILL); waitpid(m_gsrPid, &status, 0); }
        m_gsrPid = -1;
    }

    printf("[NativeRecorder] Recording stopped\n");
    UpdateStatus("Ready");
}

// ── Save clip ──────────────────────────────────────────────────────────
void NativeRecorder::SaveClip() {
    if (m_saving) { printf("[NativeRecorder] Already saving\n"); return; }
    if (m_gsrPid <= 0 || !m_recording) {
        fprintf(stderr, "[NativeRecorder] Not recording, cannot save\n");
        return;
    }

    std::thread([this]() {
        m_saving = true;

        // Snapshot existing .mp4 files before save
        std::vector<fs::path> before;
        for (std::error_code ec; const auto& e : fs::directory_iterator(m_outputDir, ec))
            if (e.path().extension() == ".mp4" || e.path().extension() == ".mkv")
                before.push_back(e.path());

        printf("[NativeRecorder] Saving clip (SIGUSR1 → pid=%d)\n", m_gsrPid);
        kill(m_gsrPid, SIGUSR1);

        // Wait up to 15s for a new file to appear
        fs::path newFile;
        for (int i = 0; i < 150 && newFile.empty(); i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::error_code ec2;
            for (const auto& e : fs::directory_iterator(m_outputDir, ec2)) {
                const auto ext = e.path().extension();
                if (ext != ".mp4" && ext != ".mkv") continue;
                bool found = false;
                for (const auto& b : before) if (b == e.path()) { found = true; break; }
                if (!found) { newFile = e.path(); break; }
            }
        }

        if (newFile.empty()) {
            fprintf(stderr, "[NativeRecorder] Clip save timeout — no new file appeared\n");
            if (m_onClipSaved) m_onClipSaved({}, false);
        } else {
            printf("[NativeRecorder] Clip saved: %s\n", newFile.c_str());
            if (m_onClipSaved) m_onClipSaved(newFile, true);
        }
        m_saving = false;
    }).detach();
}

 // ─── Utilities ─────────────────────────────────────────────────────────
bool NativeRecorder::ExecuteCommand(const std::string& cmd, std::string& out) {
    std::array<char, 256> buffer;
    out.clear();
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    while (fgets(buffer.data(), buffer.size(), pipe))
        out += buffer.data();
    return pclose(pipe) == 0;
}

void NativeRecorder::UpdateStatus(const std::string& status) {
    m_status = status;
    if (m_statusCallback) m_statusCallback(status);
}

std::string NativeRecorder::MakeTimestampName() {
    const std::time_t t = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "clip_%Y-%m-%d_%H-%M-%S.mp4", std::localtime(&t));
    return buf;
}

const std::vector<int>& NativeRecorder::GetDurationOptions() {
    static const std::vector opts = { 30, 60, 120, 180, 240, 300 };
    return opts;
}

bool NativeRecorder::IsAvailable() {
    return system("which gpu-screen-recorder > /dev/null 2>&1") == 0;
}

// ─── Dependencies & screen detection ──────────────────────────────────────
bool NativeRecorder::CheckDependencies() {
    const bool hasGSR = IsAvailable();
    printf("[NativeRecorder] gpu-screen-recorder: %s\n", hasGSR ? "OK" : "NOT FOUND");
    return hasGSR;
}

std::string NativeRecorder::GetCompositorType() {
    const char* w = getenv("WAYLAND_DISPLAY");
    const char* x = getenv("XDG_SESSION_TYPE");
    if (w || (x && strcmp(x, "wayland") == 0)) {
        if (system("pgrep -x sway     > /dev/null 2>&1") == 0) return "sway";
        if (system("pgrep -x Hyprland > /dev/null 2>&1") == 0) return "hyprland";
        return "wayland";
    }
    return "x11";
}

std::vector<ScreenInfo> NativeRecorder::GetScreens() {
    std::vector<ScreenInfo> screens;

    // Ask GSR for monitor list
    // Format: "DP-1|1920x1080@60" or "DP-1 | 1920x1080 | 60 Hz" depending on version
    std::string output;
    if (ExecuteCommand("gpu-screen-recorder --list-monitors 2>/dev/null", output) && !output.empty()) {
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.empty()) continue;
            ScreenInfo s;
            // Extract monitor name: everything before first '|' or ' '
            const size_t pipePos  = line.find('|');
            const size_t spacePos = line.find(' ');
            const size_t endPos   = std::min(pipePos, spacePos);
            s.output = (endPos != std::string::npos) ? line.substr(0, endPos) : line;
            // Trim trailing whitespace from output name
            while (!s.output.empty() && s.output.back() == ' ') s.output.pop_back();
            s.name = s.output;
            // Parse resolution from remainder: find "WxH"
            if (const size_t xp = line.find('x', endPos == std::string::npos ? 0 : endPos);
                xp != std::string::npos) {
                try {
                    // Find start of width number (scan back from x for digits)
                    size_t ws = xp;
                    while (ws > 0 && std::isdigit(line[ws-1])) --ws;
                    s.width  = std::stoi(line.substr(ws, xp - ws));
                    s.height = std::stoi(line.substr(xp + 1));
                } catch (...) {}
            }
            if (!s.output.empty()) screens.push_back(s);
        }
    }

    // Fallback: wlr-randr
    if (screens.empty()) {
        if (ExecuteCommand("wlr-randr 2>/dev/null", output)) {
            std::istringstream iss(output);
            std::string line; ScreenInfo cur; bool active = false;
            while (std::getline(iss, line)) {
                if (!line.empty() && line[0] != ' ' && line[0] != '\t') {
                    if (active && !cur.output.empty()) screens.push_back(cur);
                    cur = {}; cur.output = line.substr(0, line.find(' ')); active = false;
                }
                if (line.find("Enabled: yes") != std::string::npos) active = true;
                if (active && line.find("current") != std::string::npos)
                    sscanf(line.c_str(), " %dx%d", &cur.width, &cur.height);
            }
            if (active && !cur.output.empty()) screens.push_back(cur);
        }
    }

    // Last resort
    if (screens.empty()) {
        ScreenInfo s; s.name = "screen"; s.output = "screen";
        s.width = 1920; s.height = 1080; s.refreshRate = 60;
        screens.push_back(s);
    }

    printf("[NativeRecorder] %zu screen(s) detected\n", screens.size());
    return screens;
}