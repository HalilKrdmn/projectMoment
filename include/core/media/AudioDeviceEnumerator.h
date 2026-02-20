#pragma once

#include <string>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────
enum class AudioDeviceType {
    Input,   // microphone, line-in, etc.
    Output   // speakers, headphones, etc.
};

struct AudioDevice {
    std::string     id;          // platform-specific identifier
    std::string     displayName; // human-readable name
    AudioDeviceType type;
    bool            isDefault = false;
};
// ──────────────────────────────────────────────────────────────────────────

class AudioDeviceEnumerator {
public:
    // Returns all input (capture) devices on the system
    static std::vector<AudioDevice> GetInputDevices();

    // Returns all output (playback) devices on the system
    static std::vector<AudioDevice> GetOutputDevices();

private:
#ifdef _WIN32
    static std::vector<AudioDevice> EnumerateWASAPI(AudioDeviceType type);
#else
    static std::vector<AudioDevice> EnumeratePulse(AudioDeviceType type);
#endif
};