#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <vector>


struct AudioDevice {
    std::string id;
    std::string name;
    bool isInput;
    std::string description;
};


struct ScreenInfo {
    std::string name;
    std::string output;
    int width;
    int height;
    int x;
    int y;
    int refreshRate;
};


class NativeRecorder {
public:
    NativeRecorder();
    ~NativeRecorder();

    // Device enumeration
    std::vector<AudioDevice> GetAudioInputDevices();
    std::vector<AudioDevice> GetAudioOutputDevices();
    std::vector<ScreenInfo> GetScreens();

    // Configuration
    void SetAudioInputDevice(const std::string& deviceId);
    void SetAudioOutputDevice(const std::string& deviceId);
    void SetScreen(const std::string& output);  // Wayland output name
    void SetVideoCodec(const std::string& codec);
    void SetAudioCodec(const std::string& codec);
    void SetBitrates(int videoBitrate, int audioBitrate);
    void SetFPS(int fps);

    // Recording control
    bool StartRecording(const std::string& outputPath);
    void StopRecording();
    bool IsRecording() const { return m_isRecording; }

    // Status
    std::string GetStatus() const { return m_status; }
    float GetRecordingDuration() const;

    void SetStatusCallback(const std::function<void(const std::string&)> &callback) {
        m_statusCallback = callback;
    }

    // Check availability
    static bool CheckDependencies();
    static std::string GetCompositorType();

private:
    std::atomic<bool> m_isRecording{false};

    std::string m_audioInputDevice = "";
    std::string m_audioOutputDevice = "";
    std::string m_screenOutput = "";
    std::string m_videoCodec = "libx264";
    std::string m_audioCodec = "aac";
    int m_videoBitrate = 5000;
    int m_audioBitrate = 192;
    int m_fps = 30;

    std::string m_status = "Ready";
    std::string m_outputPath;

    pid_t m_recorderPid = -1;
    std::chrono::steady_clock::time_point m_recordingStartTime;

    std::function<void(const std::string&)> m_statusCallback;

    bool ExecuteCommand(const std::string& command, std::string& output);
    void UpdateStatus(const std::string& status);
};