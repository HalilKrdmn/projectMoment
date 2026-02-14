#pragma once
#include "imgui.h"
#include <string>

class MainScreen;

class WelcomeState {
public:
    WelcomeState() = default;
    
    void Draw(MainScreen* parent);
    
private:
    struct Cache {
        bool initialized = false;
        ImFont* lastFont = nullptr;
        ImVec2 lastAvailSize = {0, 0};
        std::string primaryText = "Welcome to Video Editor";
        std::string secondaryText = "Start organizing your videos";
        std::string tertiaryText = "Select a folder to begin";
        std::string buttonText = "Select Folder";
        ImVec2 primarySize, secondarySize, tertiarySize, buttonSize;
        float contentWidth, contentHeight;
        ImVec2 startPos;
    } m_cache;
    
    void UpdateCache();
};