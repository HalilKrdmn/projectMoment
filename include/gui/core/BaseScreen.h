#pragma once

class MainWindow;

class BaseScreen {
public:
    explicit BaseScreen(MainWindow* window) : m_manager(window) {}
    virtual ~BaseScreen() = default;

    virtual void Draw() = 0;

    MainWindow* GetManager() const { return m_manager; }

protected:
    MainWindow* m_manager;
};