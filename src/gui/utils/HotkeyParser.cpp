#include "gui/utils/HotkeyParser.h"

#include <algorithm>
#include <sstream>
#include <cctype>

ParsedHotkey HotkeyParser::Parse(const std::string& hotkeyStr) {
    ParsedHotkey result;

    if (hotkeyStr.empty()) return result;

    // Split by '+'
    std::vector<std::string> parts;
    std::stringstream ss(hotkeyStr);
    std::string part;

    while (std::getline(ss, part, '+')) {
        // Trim whitespace
        part.erase(0, part.find_first_not_of(" \t"));
        part.erase(part.find_last_not_of(" \t") + 1);

        if (!part.empty()) {
            parts.push_back(part);
        }
    }

    if (parts.empty()) return result;

    // Last part is always the key
    result.key = parts.back();

    // Check modifiers
    for (size_t i = 0; i < parts.size() - 1; i++) {
        std::string mod = parts[i];

        // Case-insensitive comparison
        std::transform(mod.begin(), mod.end(), mod.begin(), ::tolower);

        if (mod == "ctrl" || mod == "control") {
            result.ctrl = true;
        } else if (mod == "alt") {
            result.alt = true;
        } else if (mod == "shift") {
            result.shift = true;
        } else if (mod == "super" || mod == "meta" || mod == "win") {
            result.super = true;
        }
    }

    return result;
}

std::string HotkeyParser::Format(const ParsedHotkey& hotkey) {
    if (!hotkey.IsValid()) return "";

    std::string result;

    if (hotkey.ctrl)  result += "Ctrl+";
    if (hotkey.alt)   result += "Alt+";
    if (hotkey.shift) result += "Shift+";
    if (hotkey.super) result += "Super+";

    result += hotkey.key;

    return result;
}

bool HotkeyParser::IsValid(const std::string& hotkeyStr) {
    const ParsedHotkey parsed = Parse(hotkeyStr);
    return parsed.IsValid();
}

std::string HotkeyParser::GetDisplayText(const std::string& hotkeyStr) {
    const ParsedHotkey parsed = Parse(hotkeyStr);
    if (!parsed.IsValid()) return "None";

    // Display with nice symbols (optional)
    std::string result;

    if (parsed.ctrl)  result += "Ctrl + ";
    if (parsed.alt)   result += "Alt + ";
    if (parsed.shift) result += "Shift + ";
    if (parsed.super) result += "⊞ Win + ";  // Windows key symbol

    result += parsed.key;

    return result;
}

std::string ParsedHotkey::ToString() const {
    return HotkeyParser::Format(*this);
}