#pragma once
#include <vector>
#include <chrono>
#include <string>
#include <mutex>

class MainScreen;

struct LoadingLog {
    std::string message;
    float progress;
    std::chrono::system_clock::time_point timestamp;
};

class LoadingState {
public:
    LoadingState() = default;
    
    void Draw(MainScreen* parent);
    void AddLog(const std::string& message, float progress);
    void SetProgress(const float progress) { m_progress = progress; }
    float GetProgress() const { return m_progress; }
    void Clear();
    
private:
    std::vector<LoadingLog> m_logs;
    float m_progress = 0.0f;
    std::mutex m_logMutex;
};