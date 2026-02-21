#include "core/recording/NativeRecorder.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>
#include <thread>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

// ──────────────────────────────────────────────────────────────────────────
const std::vector<int>& NativeRecorder::GetDurationOptions() {
    static const std::vector opts = { 30, 60, 120, 180, 240, 300 };
    return opts;
}

bool NativeRecorder::ExecuteCommand(const std::string& cmd, std::string& out) {
    std::array<char, 128> buffer;
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

fs::path NativeRecorder::MakeTempDir() {
    const char* tmp = std::getenv("XDG_RUNTIME_DIR");
    if (!tmp) tmp = "/tmp";
    fs::path dir = fs::path(tmp) / "projectMoment_segments";
    fs::create_directories(dir);
    return dir;
}

std::string NativeRecorder::MakeTimestampName() {
    const std::time_t t = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "clip_%Y-%m-%d_%H-%M-%S.mp4", std::localtime(&t));
    return buf;
}

std::string NativeRecorder::BuildWfRecorderCmd(
    const std::string &screenOutput,
    const std::vector<AudioTrack> &audioTracks,
    const std::string &videoCodec,
    const int fps,
    const std::string &outputPath) {

    std::ostringstream cmd;
    cmd << "wf-recorder";

    if (!screenOutput.empty())
        cmd << " -o " << screenOutput;

    if (!audioTracks.empty()) {
        for (const auto& t : audioTracks) {
            cmd << " -a";
            if (!t.device.empty() && t.device != "default")
                cmd << "=" << t.device;
        }
    } else {
        cmd << " -a";
    }

    cmd << " -c " << videoCodec;
    cmd << " -r " << fps;
    cmd << " --bframes 0 -p preset=ultrafast";
    cmd << " -f " << outputPath;
    return cmd.str();
}
// ──────────────────────────────────────────────────────────────────────────

NativeRecorder::NativeRecorder() {
    m_tempDir   = MakeTempDir();
    m_outputDir = fs::path(std::getenv("HOME") ? std::getenv("HOME") : "/tmp") / "Videos";

    if (!CheckDependencies())
        UpdateStatus("[NativeRecorder] Warning: wf-recorder or Pipewire is missing");
}

NativeRecorder::~NativeRecorder() {
    StopRecording();
    std::error_code ec;
    fs::remove_all(m_tempDir, ec);
}

// ─── Configuration ──────────────────────────────────────────────────────────
void NativeRecorder::SetAudioTracks(const std::vector<AudioTrack>& tracks)          { m_audioTracks  = tracks;    }
void NativeRecorder::SetScreen(const std::string& output)                           { m_screenOutput = output; printf("[NativeRecorder] Screen → %s\n", output.c_str()); }
void NativeRecorder::SetVideoCodec(const std::string& codec)                        { m_videoCodec   = codec;    }
void NativeRecorder::SetAudioCodec(const std::string& codec)                        { m_audioCodec   = codec;    }
void NativeRecorder::SetBitrates(const int videoBitrate, const int audioBitrate)    { m_videoBitrate = videoBitrate; m_audioBitrate = audioBitrate; }
void NativeRecorder::SetFPS(const int fps)                                          { m_fps          = fps;  }
void NativeRecorder::SetClipDuration(const int seconds)                             { m_clipDuration = seconds;    }

void NativeRecorder::SetOutputDirectory(const fs::path& dir) {
    m_outputDir = dir;
    fs::create_directories(dir);
}

// ──── Recording control — Start / Stop ───────────────────────────────────────
bool NativeRecorder::StartRecording() {
    if (m_recording) {
        printf("[NativeRecorder] It's already being recorded.\n");
        return true;
    }

    for (std::error_code ec; auto& e : fs::directory_iterator(m_tempDir, ec))
        fs::remove(e.path(), ec);
    {
        std::lock_guard lock(m_segMutex);
        m_segments.clear();
    }

    m_recording = true;
    m_thread    = std::thread(&NativeRecorder::SegmentLoop, this);

    printf("[NativeRecorder] Registration has begun — clip: %ds, marj: %ds\n",
           m_clipDuration, m_marginSec);
    UpdateStatus("Recording in progress...");
    return true;
}

void NativeRecorder::StopRecording() {
    if (!m_recording) return;
    m_recording = false;

    if (m_thread.joinable()) m_thread.join();
    StopCurrentSegment();

    printf("[NativeRecorder] Recording stopped\n");
    UpdateStatus("Ready");
}

// ─── Segment cycle ───────────────────────────────────────────────────────────
void NativeRecorder::SegmentLoop() {
    while (m_recording) {
        if (!StartNextSegment()) {
            fprintf(stderr, "[NativeRecorder] Segment could not be started, waiting for 2 seconds\n");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        const int slices = m_segDuration * 10;
        for (int i = 0; i < slices && m_recording; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        StopCurrentSegment();
        PruneOldSegments();
    }
}

bool NativeRecorder::StartNextSegment() {
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    char name[64];
    snprintf(name, sizeof(name), "seg_%lld.mp4", static_cast<long long>(now_ms));
    m_currentSegPath = m_tempDir / name;

    const std::string command = "exec " + BuildWfRecorderCmd(
        m_screenOutput, m_audioTracks, m_videoCodec, m_fps, m_currentSegPath.string());

    printf("[NativeRecorder] Segment: %s\n", m_currentSegPath.filename().c_str());
    printf("[NativeRecorder] CMD: %s\n", command.c_str());

    m_segPid = fork();
    if (m_segPid == 0) {
        if (const int devNull = open("/dev/null", O_WRONLY); devNull >= 0) { dup2(devNull, STDERR_FILENO); close(devNull); }
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        exit(1);
    }
    if (m_segPid < 0) {
        fprintf(stderr, "[NativeRecorder] fork() failed\n");
        return false;
    }

    ReplaySegment seg;
    seg.path        = m_currentSegPath;
    seg.createdAt   = std::chrono::steady_clock::now();
    seg.durationSec = static_cast<float>(m_segDuration);
    {
        std::lock_guard lock(m_segMutex);
        m_segments.push_back(std::move(seg));
    }
    return true;
}

void NativeRecorder::StopCurrentSegment() {
    if (m_segPid <= 0) return;
    kill(m_segPid, SIGINT);
    int status, waited = 0;
    while (waited++ < 30) {
        if (waitpid(m_segPid, &status, WNOHANG) > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (waited >= 30) { kill(m_segPid, SIGKILL); waitpid(m_segPid, &status, 0); }
    m_segPid = -1;
}

void NativeRecorder::PruneOldSegments() {
    const int keepSec = m_clipDuration + m_marginSec;
    const auto now    = std::chrono::steady_clock::now();
    std::lock_guard lock(m_segMutex);
    while (!m_segments.empty()) {
        const auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - m_segments.front().createdAt).count();
        if (age > keepSec) {
            std::error_code ec;
            fs::remove(m_segments.front().path, ec);
            m_segments.pop_front();
        } else break;
    }
}

 // ─── Save clip ────────────────────────────────────────────────────────────────
void NativeRecorder::SaveClip() {
    if (m_saving) {
        printf("[NativeRecorder] It's already being recorded, skipped\n");
        return;
    }

    std::thread([this]() {
        m_saving = true;
        fs::create_directories(m_outputDir);
        const fs::path outPath = m_outputDir / MakeTimestampName();
        printf("[NativeRecorder] Clip is being recorded → %s\n", outPath.c_str());

        const bool ok = MergeSegments(outPath);

        if (ok)   printf("[NativeRecorder] The clip has been recorded.: %s\n", outPath.c_str());
        else      fprintf(stderr, "[NativeRecorder] Clip recording FAILED\n");

        if (m_onClipSaved) m_onClipSaved(outPath, ok);
        m_saving = false;
    }).detach();
}

bool NativeRecorder::MergeSegments(const fs::path& out) {
    std::vector<ReplaySegment> segments;
    {
        std::lock_guard lock(m_segMutex);
        segments.assign(m_segments.begin(), m_segments.end());
    }

    if (segments.empty()) {
        fprintf(stderr, "[NativeRecorder] No segments to merge\n");
        return false;
    }

    const fs::path listFile = m_tempDir / "concat.txt";
    {
        std::ofstream f(listFile);
        if (!f.is_open()) return false;

        int written = 0;
        for (const auto& seg : segments) {
            if (seg.path == m_currentSegPath) continue;
            std::error_code ec;
            if (const auto size = fs::file_size(seg.path, ec); ec || size == 0) continue;
            f << "file '" << seg.path.string() << "'\n";
            ++written;
        }
        if (written == 0) {
            fprintf(stderr, "[NativeRecorder] No completed segments\n");
            return false;
        }
        printf("[NativeRecorder] %d segment is being merged\n", written);
    }

    std::ostringstream cmd;
    cmd << "ffmpeg -y -loglevel error"
        << " -f concat -safe 0"
        << " -sseof -" << m_clipDuration
        << " -i " << listFile.string()
        << " -c copy"
        << " " << out.string();

    printf("[NativeRecorder] Concat: %s\n", cmd.str().c_str());
    const int ret = system(cmd.str().c_str());

    std::error_code ec;
    fs::remove(listFile, ec);
    return (ret == 0) && fs::exists(out);
}

// ─── Info ───────────────────────────────────────────────────────────────────────
float NativeRecorder::GetBufferedSeconds() const {
    std::lock_guard lock(m_segMutex);
    float total = 0.0f;
    for (const auto& s : m_segments) total += s.durationSec;
    return total;
}

int NativeRecorder::GetSegmentCount() const {
    std::lock_guard lock(m_segMutex);
    return static_cast<int>(m_segments.size());
}

// ─── Dependency / compositor control ───────────────────────────────────────────────────────────────────────
bool NativeRecorder::CheckDependencies() {
    const bool hasWfRecorder = system("which wf-recorder > /dev/null 2>&1") == 0;
    const bool hasPipewire   = system("which pw-cli > /dev/null 2>&1") == 0;
    const bool hasPulseAudio = system("which pactl > /dev/null 2>&1") == 0;

    printf("[NativeRecorder] Addiction control:\n");
    printf("  wf-recorder : %s\n", hasWfRecorder ? "✓" : "✗");
    printf("  PipeWire    : %s\n", hasPipewire   ? "✓" : "✗");
    printf("  PulseAudio  : %s\n", hasPulseAudio ? "✓" : "✗");

    return hasWfRecorder && (hasPipewire || hasPulseAudio);
}

std::string NativeRecorder::GetCompositorType() {
    const char* waylandDisplay = getenv("WAYLAND_DISPLAY");

    if (const char* xdgSessionType = getenv("XDG_SESSION_TYPE"); waylandDisplay ||
                                     (xdgSessionType && strcmp(xdgSessionType, "wayland") == 0)) {
        if (system("pgrep -x sway > /dev/null 2>&1")     == 0) return "sway";
        if (system("pgrep -x Hyprland > /dev/null 2>&1") == 0) return "hyprland";
        if (system("pgrep -x wayfire > /dev/null 2>&1")  == 0) return "wayfire";
        return "wayland";
    }
    return "x11";
}

std::vector<ScreenInfo> NativeRecorder::GetScreens() {
    std::vector<ScreenInfo> screens;
    std::string output;
    const std::string compositor = GetCompositorType();

    auto pushDefault = [&](const std::string& outName) {
        ScreenInfo s;
        s.name = "Primary Display"; s.output = outName;
        s.width = 1920; s.height = 1080; s.refreshRate = 60;
        screens.push_back(s);
    };

    if (compositor == "sway") {
        if (ExecuteCommand("swaymsg -t get_outputs -r", output)) pushDefault("HDMI-A-1");
    } else if (compositor == "hyprland") {
        if (ExecuteCommand("hyprctl monitors -j", output)) pushDefault("HDMI-A-1");
    } else {
        if (ExecuteCommand("wlr-randr", output)) {
            std::istringstream iss(output);
            std::string line;
            ScreenInfo cur;
            bool active = false;
            while (std::getline(iss, line)) {
                if (line.find("Enabled: yes") != std::string::npos) { active = true; }
                else if (active && line.find("current") != std::string::npos) {
                    sscanf(line.c_str(), " %dx%d", &cur.width, &cur.height);
                    cur.name = "Display"; cur.output = "AUTO";
                    screens.push_back(cur); active = false;
                }
            }
        }
    }

    if (screens.empty()) pushDefault("");
    printf("[NativeRecorder] %zu screen found\n", screens.size());
    return screens;
}