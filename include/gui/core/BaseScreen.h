#pragma once

class MainWindow;

class BaseScreen {
public:
    explicit BaseScreen(MainWindow* window) : m_mainWindow(window) {}
    virtual ~BaseScreen() = default;

    virtual void Draw() = 0;

protected:
    MainWindow* m_mainWindow;
};