#include "core/media/AudioDeviceEnumerator.h"

#include <cstdio>
#include <cstring>
#include <sstream>

//  WINDOWS  —  WASAPI (NOT TESTED)
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

static std::string WideToUTF8(const wchar_t* wide) {
    if (!wide) return {};
    const int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    std::string out(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out.data(), len, nullptr, nullptr);
    return out;
}

std::vector<AudioDevice> AudioDeviceEnumerator::EnumerateWASAPI(AudioDeviceType type) {
    std::vector<AudioDevice> result;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                CLSCTX_ALL, IID_PPV_ARGS(&enumerator)))) return result;

    const EDataFlow flow = (type == AudioDeviceType::Input) ? eCapture : eRender;

    ComPtr<IMMDevice> defaultDev;
    std::string defaultId;
    if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(flow, eConsole, &defaultDev))) {
        LPWSTR wid = nullptr;
        defaultDev->GetId(&wid);
        if (wid) { defaultId = WideToUTF8(wid); CoTaskMemFree(wid); }
    }

    ComPtr<IMMDeviceCollection> collection;
    if (FAILED(enumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &collection))) return result;

    UINT count = 0;
    collection->GetCount(&count);

    // Prepend default
    {
        AudioDevice def;
        def.id          = "default";
        def.displayName = (type == AudioDeviceType::Input) ? "Default Input" : "Default Output";
        def.type        = type;
        def.isDefault   = true;
        result.push_back(std::move(def));
    }

    for (UINT i = 0; i < count; ++i) {
        ComPtr<IMMDevice> device;
        if (FAILED(collection->Item(i, &device))) continue;

        LPWSTR wid = nullptr;
        device->GetId(&wid);
        std::string id = wid ? WideToUTF8(wid) : "";
        if (wid) CoTaskMemFree(wid);

        ComPtr<IPropertyStore> props;
        if (FAILED(device->OpenPropertyStore(STGM_READ, &props))) continue;

        PROPVARIANT nameVar;
        PropVariantInit(&nameVar);
        props->GetValue(PKEY_Device_FriendlyName, &nameVar);
        std::string name = (nameVar.vt == VT_LPWSTR) ? WideToUTF8(nameVar.pwszVal) : id;
        PropVariantClear(&nameVar);

        AudioDevice dev;
        dev.id          = id;
        dev.displayName = name;
        dev.type        = type;
        dev.isDefault   = (id == defaultId);
        result.push_back(std::move(dev));
    }
    return result;
}

std::vector<AudioDevice> AudioDeviceEnumerator::GetInputDevices()  { return EnumerateWASAPI(AudioDeviceType::Input);  }
std::vector<AudioDevice> AudioDeviceEnumerator::GetOutputDevices() { return EnumerateWASAPI(AudioDeviceType::Output); }

#else

//  LINUX  —  PulseAudio / PipeWire

#include <map>

// Pactl entry elements
struct PactlEntry {
    std::string name;
    std::string description;
    bool        isMonitor = false;
};

// Run a command such as `pactl list sources` or `pactl list sinks`,
// read the output line by line, and convert each audio device into a `PactlEntry` struct.
static std::vector<PactlEntry> ParsePactlLong(const char* pactlArg) {
    std::vector<PactlEntry> entries;

    const std::string cmd = std::string("pactl list ") + pactlArg + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return entries;

    char buf[1024];
    PactlEntry current;
    bool inEntry = false;

    auto flush = [&]() {
        if (inEntry && !current.name.empty()) {
            current.isMonitor = (current.name.size() >= 8 &&
                                 current.name.substr(current.name.size() - 8) == ".monitor");
            entries.push_back(current);
        }
        current = {};
        inEntry = false;
    };

    while (fgets(buf, sizeof(buf), pipe)) {
        std::string line(buf);
        if (line.find("Source #") != std::string::npos ||
            line.find("Sink #")   != std::string::npos) {
            flush();
            inEntry = true;
            continue;
        }

        if (!inEntry) continue;

        if (const auto namePos = line.find("Name:"); namePos != std::string::npos) {
            current.name = line.substr(namePos + 5);
            while (!current.name.empty() && (current.name.back() == '\n' ||
                   current.name.back() == '\r' || current.name.back() == ' '))
                current.name.pop_back();
            if (const auto start = current.name.find_first_not_of(" \t"); start != std::string::npos) current.name = current.name.substr(start);
            continue;
        }

        if (const auto descPos = line.find("Description:"); descPos != std::string::npos) {
            current.description = line.substr(descPos + 12);
            while (!current.description.empty() && (current.description.back() == '\n' ||
                   current.description.back() == '\r' || current.description.back() == ' '))
                current.description.pop_back();
            if (const auto start = current.description.find_first_not_of(" \t"); start != std::string::npos) current.description = current.description.substr(start);
        }
    }
    flush();
    pclose(pipe);
    return entries;
}

// Find the default devices. (pactl info)
static std::string GetDefaultDevice(const char* infoKey) {
    FILE* pipe = popen("pactl info 2>/dev/null", "r");
    if (!pipe) return {};
    char buf[512];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string line(buf);
        if (const auto pos = line.find(infoKey); pos != std::string::npos) {
            result = line.substr(pos + strlen(infoKey));
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
                result.pop_back();
            if (const auto s = result.find_first_not_of(" \t"); s != std::string::npos) result = result.substr(s);
            break;
        }
    }
    pclose(pipe);
    return result;
}

// Lists the audio input/output devices on PulseAudio and converts them to AudioDevice objects.
std::vector<AudioDevice> AudioDeviceEnumerator::EnumeratePulse(const AudioDeviceType type) {
    std::vector<AudioDevice> result;

    const bool   isInput   = (type == AudioDeviceType::Input);
    const char*  pactlArg  = isInput ? "sources" : "sinks";
    const char*  infoKey   = isInput ? "Default Source:" : "Default Sink:";

    const std::string defaultName = GetDefaultDevice(infoKey);
    const auto entries = ParsePactlLong(pactlArg);

    {
        AudioDevice def;
        def.id          = "default";
        def.displayName = isInput ? "Default Input" : "Default Output";
        def.type        = type;
        def.isDefault   = true;
        result.push_back(std::move(def));
    }

    for (const auto&[name, description, isMonitor] : entries) {
        if (isInput && isMonitor) continue;

        AudioDevice dev;
        dev.id          = name;
        // For better readability, read the explanation.
        // If there is no explanation, use the raw name.
        dev.displayName = !description.empty() ? description : name;
        dev.type        = type;
        dev.isDefault   = (name == defaultName);
        result.push_back(std::move(dev));
    }

    return result;
}

std::vector<AudioDevice> AudioDeviceEnumerator::GetInputDevices()  { return EnumeratePulse(AudioDeviceType::Input);  }
std::vector<AudioDevice> AudioDeviceEnumerator::GetOutputDevices() { return EnumeratePulse(AudioDeviceType::Output); }

#endif