#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "data/VideoInfo.h"

class BaseScreen;

enum class ApplicationState {
    MAIN,
    SETTINGS,
    EDITING
};

class MainWindow {
public:
    MainWindow(int width, int height, const char* title);
    ~MainWindow();

    int Run() const;
    void SetApplicationState(ApplicationState newState);

    ApplicationState GetCurrentState() const { return m_currentState; }
    GLFWwindow* GetWindow() const { return window; }

    void SwitchToEditingScreen(const VideoInfo& video);

    const VideoInfo& GetSelectedVideo() const { return m_selectedVideo; }

private:
    GLFWwindow* window;

    BaseScreen* m_currentScreen = nullptr;
    BaseScreen* m_mainScreen = nullptr;
    BaseScreen* m_editingScreen = nullptr;

    ApplicationState m_currentState;

    VideoInfo m_selectedVideo;
};