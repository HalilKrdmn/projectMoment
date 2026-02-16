#pragma once

#include <string>
#include <functional>


enum class ExportStatus {
    IDLE,
    EXPORTING,
    SUCCESS,
    FAILED
};

struct ExportSettings {
    std::string inputPath;
    std::string outputFilename;
    float startTime;
    float endTime;
    int maxSizeMB = 0;
};


class VideoExporter {
public:
    VideoExporter() = default;
    ~VideoExporter();

    void StartExport(const ExportSettings& settings,
                     std::function<void(float)> progressCallback = nullptr,
                     std::function<void(bool)> completeCallback = nullptr,
                     std::function<void(const std::string&)> logCallback = nullptr);

    ExportStatus GetStatus() const { return m_status; }
    float GetProgress() const { return m_progress; }
    std::string GetErrorMessage() const { return m_errorMessage; }
    std::string GetOutputPath() const { return m_outputPath; }

    void CancelExport();
    void Reset();

private:
    void DoExport(const ExportSettings &settings,
                  const std::function<void(float)> &progressCallback,
                  const std::function<void(bool)> &completeCallback,
                  const std::function<void(const std::string&)> &logCallback);

    void KillFFmpegProcess();

    static std::string GenerateOutputPath(const std::string& inputPath,
                                          const std::string& filename);

    int CalculateBitrate(float durationSeconds, int maxSizeMB);

    ExportStatus m_status = ExportStatus::IDLE;
    float m_progress = 0.0f;
    std::string m_errorMessage;
    std::string m_outputPath;
    bool m_shouldCancel = false;
    FILE* m_pipe = nullptr;
};