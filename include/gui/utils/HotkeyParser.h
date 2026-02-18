#pragma once

#include <string>
#include <vector>

struct ParsedHotkey {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool super = false;  // Windows key / Meta
    std::string key;
    
    bool IsValid() const { return !key.empty(); }
    std::string ToString() const;
};

class HotkeyParser {
public:
    // Parse "Alt+C" -> {alt=true, key="C"}
    static ParsedHotkey Parse(const std::string& hotkeyStr);
    // Format {alt=true, key="C"} -> "Alt+C"
    static std::string Format(const ParsedHotkey& hotkey);
    
    // Validate hotkey string
    static bool IsValid(const std::string& hotkeyStr);
    // Get user-friendly display text
    static std::string GetDisplayText(const std::string& hotkeyStr);
};