#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class BaseScreen;

enum class ApplicationState {
    LOADING,
    MAIN_MENU,
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

private:
    GLFWwindow* window;
    BaseScreen* m_currentScreen;
    ApplicationState m_currentState;
};