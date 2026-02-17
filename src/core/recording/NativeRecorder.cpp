#include "core/recording/NativeRecorder.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <array>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>

NativeRecorder::NativeRecorder() {
    if (!CheckDependencies()) {
        UpdateStatus("Warning: Missing dependencies (wf-recorder or pipewire)");
    }
}

NativeRecorder::~NativeRecorder() {
    StopRecording();
}

bool NativeRecorder::CheckDependencies() {
    const bool hasWfRecorder = system("which wf-recorder > /dev/null 2>&1") == 0;

    const bool hasPipewire = system("which pw-cli > /dev/null 2>&1") == 0;

    const bool hasPulseAudio = system("which pactl > /dev/null 2>&1") == 0;

    printf("[NativeRecorder] Dependencies check:\n");
    printf("  wf-recorder: %s\n", hasWfRecorder ? "✓" : "✗");
    printf("  PipeWire: %s\n", hasPipewire ? "✓" : "✗");
    printf("  PulseAudio: %s\n", hasPulseAudio ? "✓" : "✗");

    return hasWfRecorder && (hasPipewire || hasPulseAudio);
}

std::string NativeRecorder::GetCompositorType() {
    const char* waylandDisplay = getenv("WAYLAND_DISPLAY");

    if (const char* xdgSessionType = getenv("XDG_SESSION_TYPE"); waylandDisplay || (xdgSessionType && strcmp(xdgSessionType, "wayland") == 0)) {
        if (system("pgrep -x sway > /dev/null 2>&1") == 0) return "sway";
        if (system("pgrep -x Hyprland > /dev/null 2>&1") == 0) return "hyprland";
        if (system("pgrep -x wayfire > /dev/null 2>&1") == 0) return "wayfire";
        return "wayland";
    }

    return "x11";
}

bool NativeRecorder::ExecuteCommand(const std::string& command, std::string& output) {
    std::array<char, 128> buffer;
    output.clear();

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    const int returnCode = pclose(pipe);
    return returnCode == 0;
}

std::vector<AudioDevice> NativeRecorder::GetAudioInputDevices() {
    std::vector<AudioDevice> devices;

    if (std::string output; ExecuteCommand("pw-cli ls Node 2>/dev/null | grep -A 5 'Audio/Source'", output)) {
        printf("[NativeRecorder] Using PipeWire for audio input devices\n");
    }
    else if (ExecuteCommand("pactl list sources short", output)) {
        printf("[NativeRecorder] Using PulseAudio for audio input devices\n");

        std::istringstream iss(output);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.empty()) continue;

            std::istringstream lineStream(line);
            std::string index, name, driver, format, channels, state;

            lineStream >> index >> name >> driver >> format >> channels >> state;

            AudioDevice device;
            device.id = name;
            device.name = name;
            device.isInput = true;
            device.description = driver;

            devices.push_back(device);
        }
    }

    if (devices.empty()) {
        devices.push_back({"default", "Default Input", true, "System default"});
    }

    printf("[NativeRecorder] Found %zu audio input devices\n", devices.size());
    return devices;
}

std::vector<AudioDevice> NativeRecorder::GetAudioOutputDevices() {
    std::vector<AudioDevice> devices;

    if (std::string output; ExecuteCommand("pactl list sinks short", output)) {
        std::istringstream iss(output);
        std::string line;

        while (std::getline(iss, line)) {
            if (line.empty()) continue;

            std::istringstream lineStream(line);
            std::string index, name, driver, format, channels, state;

            lineStream >> index >> name >> driver >> format >> channels >> state;

            AudioDevice device;
            device.id = name;
            device.name = name;
            device.isInput = false;
            device.description = driver;

            devices.push_back(device);
        }
    }

    if (devices.empty()) {
        devices.push_back({"default", "Default Output", false, "System default"});
    }

    printf("[NativeRecorder] Found %zu audio output devices\n", devices.size());
    return devices;
}

std::vector<ScreenInfo> NativeRecorder::GetScreens() {
    std::vector<ScreenInfo> screens;
    std::string output;

    if (std::string compositor = GetCompositorType(); compositor == "sway") {
        if (ExecuteCommand("swaymsg -t get_outputs -r", output)) {
            printf("[NativeRecorder] Sway outputs detected\n");

            ScreenInfo screen;
            screen.name = "Primary Display";
            screen.output = "HDMI-A-1";
            screen.width = 1920;
            screen.height = 1080;
            screen.x = 0;
            screen.y = 0;
            screen.refreshRate = 60;
            screens.push_back(screen);
        }
    }
    else if (compositor == "hyprland") {
        if (ExecuteCommand("hyprctl monitors -j", output)) {
            printf("[NativeRecorder] Hyprland outputs detected\n");

            ScreenInfo screen;
            screen.name = "Primary Display";
            screen.output = "HDMI-A-1";
            screen.width = 1920;
            screen.height = 1080;
            screen.x = 0;
            screen.y = 0;
            screen.refreshRate = 60;
            screens.push_back(screen);
        }
    }
    else {
        if (ExecuteCommand("wlr-randr", output)) {
            printf("[NativeRecorder] Using wlr-randr for display detection\n");

            std::istringstream iss(output);
            std::string line;
            ScreenInfo currentScreen;
            bool hasScreen = false;

            while (std::getline(iss, line)) {
                if (line.find("Enabled: yes") != std::string::npos) {
                    hasScreen = true;
                } else if (hasScreen && line.find("current") != std::string::npos) {
                    // Parse resolution: "1920x1080 @ 60.000 Hz"
                    sscanf(line.c_str(), " %dx%d", &currentScreen.width, &currentScreen.height);
                    currentScreen.name = "Display";
                    currentScreen.output = "AUTO";
                    screens.push_back(currentScreen);
                    hasScreen = false;
                }
            }
        }
    }

    // Fallback
    if (screens.empty()) {
        ScreenInfo screen;
        screen.name = "Default Display";
        screen.output = "";
        screen.width = 1920;
        screen.height = 1080;
        screen.x = 0;
        screen.y = 0;
        screen.refreshRate = 60;
        screens.push_back(screen);
    }

    printf("[NativeRecorder] Found %zu screens\n", screens.size());
    return screens;
}

bool NativeRecorder::StartRecording(const std::string& outputPath) {
    if (m_isRecording) {
        printf("[NativeRecorder] Already recording\n");
        return false;
    }

    m_outputPath = outputPath;

    std::ostringstream cmd;
    cmd << "wf-recorder";

    // Output device
    if (!m_screenOutput.empty()) {
        cmd << " -o " << m_screenOutput;
    }

    // Audio input (microphone)
    if (!m_audioInputDevice.empty()) {
        cmd << " -a";
        if (m_audioInputDevice != "default") {
            cmd << "=" << m_audioInputDevice;
        }
    }

    // Codec settings
    cmd << " -c " << m_videoCodec;
    cmd << " -r " << m_fps;

    // Bitrate
    cmd << " --bframes 0";  // Low latency
    cmd << " -p preset=ultrafast";

    // Output file
    cmd << " -f " << outputPath;

    // Background process
    cmd << " &";

    const std::string command = cmd.str();
    printf("[NativeRecorder] Starting recording: %s\n", command.c_str());

    // Fork process
    m_recorderPid = fork();

    if (m_recorderPid == 0) {
        // Child process
        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        exit(1);
    }
    if (m_recorderPid > 0) {
        // Parent process
        m_isRecording = true;
        m_recordingStartTime = std::chrono::steady_clock::now();
        UpdateStatus("Recording...");
        return true;
    }
    // Fork failed
    UpdateStatus("Failed to start recording");
    return false;
}

void NativeRecorder::StopRecording() {
    if (!m_isRecording) return;

    printf("[NativeRecorder] Stopping recording (PID: %d)\n", m_recorderPid);

    if (m_recorderPid > 0) {
        kill(m_recorderPid, SIGINT);

        int status;
        waitpid(m_recorderPid, &status, 0);

        m_recorderPid = -1;
    }

    m_isRecording = false;
    UpdateStatus("Recording stopped");

    printf("[NativeRecorder] Recording saved to: %s\n", m_outputPath.c_str());
}

float NativeRecorder::GetRecordingDuration() const {
    if (!m_isRecording) return 0.0f;

    const auto now = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_recordingStartTime);
    return static_cast<float>(duration.count());
}

void NativeRecorder::UpdateStatus(const std::string& status) {
    m_status = status;
    if (m_statusCallback) {
        m_statusCallback(status);
    }
}

void NativeRecorder::SetAudioInputDevice(const std::string& deviceId) {
    m_audioInputDevice = deviceId;
    printf("[NativeRecorder] Audio input device set to: %s\n", deviceId.c_str());
}

void NativeRecorder::SetAudioOutputDevice(const std::string& deviceId) {
    m_audioOutputDevice = deviceId;
    printf("[NativeRecorder] Audio output device set to: %s\n", deviceId.c_str());
}

void NativeRecorder::SetScreen(const std::string& output) {
    m_screenOutput = output;
    printf("[NativeRecorder] Screen output set to: %s\n", output.c_str());
}

void NativeRecorder::SetVideoCodec(const std::string& codec) {
    m_videoCodec = codec;
}

void NativeRecorder::SetAudioCodec(const std::string& codec) {
    m_audioCodec = codec;
}

void NativeRecorder::SetBitrates(const int videoBitrate, const int audioBitrate) {
    m_videoBitrate = videoBitrate;
    m_audioBitrate = audioBitrate;
}

void NativeRecorder::SetFPS(const int fps) {
    m_fps = fps;
}