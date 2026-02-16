#include "core/media/VideoExporter.h"

#include <cstdio>
#include <filesystem>
#include <thread>

VideoExporter::~VideoExporter() {
    KillFFmpegProcess();
}

void VideoExporter::StartExport(const ExportSettings& settings,
                                std::function<void(float)> progressCallback,
                                std::function<void(bool)> completeCallback,
                                std::function<void(const std::string&)> logCallback) {
    if (m_status == ExportStatus::EXPORTING) {
        printf("[VideoExporter] Already exporting!\n");
        return;
    }

    KillFFmpegProcess();

    m_status = ExportStatus::EXPORTING;
    m_progress = 0.0f;
    m_errorMessage.clear();
    m_shouldCancel = false;

    m_outputPath = GenerateOutputPath(settings.inputPath, settings.outputFilename);

    std::thread([this, settings, progressCallback, completeCallback, logCallback]() {
        DoExport(settings, progressCallback, completeCallback, logCallback);
    }).detach();
}

void VideoExporter::CancelExport() {
    m_shouldCancel = true;
    KillFFmpegProcess();
}

void VideoExporter::Reset() {
    KillFFmpegProcess();

    m_status = ExportStatus::IDLE;
    m_progress = 0.0f;
    m_errorMessage.clear();
    m_outputPath.clear();
    m_shouldCancel = false;
}

void VideoExporter::DoExport(const ExportSettings& settings,
                             const std::function<void(float)> &progressCallback,
                             const std::function<void(bool)> &completeCallback,
                             const std::function<void(const std::string&)> &logCallback) {
    char command[2048];
    const float duration = settings.endTime - settings.startTime;

    if (settings.maxSizeMB > 0) {
        const int bitrate = CalculateBitrate(duration, settings.maxSizeMB);

        snprintf(command, sizeof(command),
                 "ffmpeg -y -ss %.2f -i \"%s\" -t %.2f "
                 "-b:v %dk -maxrate %dk -bufsize %dk "
                 "\"%s\" 2>&1",
                 settings.startTime,
                 settings.inputPath.c_str(),
                 duration,
                 bitrate,
                 bitrate,
                 bitrate * 2,
                 m_outputPath.c_str());
    } else {
        snprintf(command, sizeof(command),
                 "ffmpeg -y -ss %.2f -i \"%s\" -t %.2f "
                 "-c copy "
                 "\"%s\" 2>&1",
                 settings.startTime,
                 settings.inputPath.c_str(),
                 duration,
                 m_outputPath.c_str());
    }

    printf("[VideoExporter] Command: %s\n", command);

    if (logCallback) {
        logCallback("Starting FFmpeg...");
    }

    if (progressCallback) {
        progressCallback(0.0f);
    }

    m_progress = 0.0f;

    m_pipe = popen(command, "r");
    if (!m_pipe) {
        m_status = ExportStatus::FAILED;
        m_errorMessage = "Failed to start FFmpeg";
        if (completeCallback) completeCallback(false);
        if (logCallback) logCallback("ERROR: Failed to start FFmpeg");
        return;
    }

    char buffer[512];
    bool wasCancelled = false;

    while (!m_shouldCancel && fgets(buffer, sizeof(buffer), m_pipe) != nullptr) {
        std::string line(buffer);

        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        if (line.empty()) continue;

        if (logCallback && !m_shouldCancel) {
            logCallback(line);
        }

        if (const size_t timePos = line.find("time="); timePos != std::string::npos) {
            const size_t start = timePos + 5;
            size_t end = line.find(' ', start);
            if (end == std::string::npos) end = line.length();

            std::string timeStr = line.substr(start, end - start);

            int hours = 0, mins = 0;
            float secs = 0.0f;
            if (sscanf(timeStr.c_str(), "%d:%d:%f", &hours, &mins, &secs) == 3) {
                const float currentTime = hours * 3600 + mins * 60 + secs;
                m_progress = std::min(currentTime / duration, 1.0f);

                if (progressCallback && !m_shouldCancel) {
                    progressCallback(m_progress);
                }
            }
        }
    }

    if (m_shouldCancel) {
        wasCancelled = true;
    }

    const int result = pclose(m_pipe);
    m_pipe = nullptr;

    if (wasCancelled) {
        m_status = ExportStatus::FAILED;
        m_errorMessage = "Export cancelled";
        m_progress = 0.0f;
        if (completeCallback) completeCallback(false);
        if (logCallback) logCallback("✗ Export cancelled by user");

        if (std::filesystem::exists(m_outputPath)) {
            std::filesystem::remove(m_outputPath);
            printf("[VideoExporter] Deleted incomplete file: %s\n", m_outputPath.c_str());
        }
    } else if (result == 0) {
        m_status = ExportStatus::SUCCESS;
        m_progress = 1.0f;
        if (progressCallback) progressCallback(1.0f);
        if (completeCallback) completeCallback(true);
        if (logCallback) logCallback("✓ Export successful!");
    } else {
        m_status = ExportStatus::FAILED;
        m_errorMessage = "FFmpeg failed with code: " + std::to_string(result);
        m_progress = 0.0f;
        if (completeCallback) completeCallback(false);
        if (logCallback) logCallback("✗ Export failed (code " + std::to_string(result) + ")");
    }
}

std::string VideoExporter::GenerateOutputPath(const std::string& inputPath,
                                               const std::string& filename) {
    const size_t lastSlash = inputPath.find_last_of("/\\");
    const std::string directory = (lastSlash != std::string::npos)
        ? inputPath.substr(0, lastSlash + 1)
        : "";

    const std::string exportDir = directory + "export/";
    std::filesystem::create_directories(exportDir);

    std::string outputPath = exportDir + filename;

    printf("[VideoExporter] Output path: %s\n", outputPath.c_str());
    return outputPath;
}

int VideoExporter::CalculateBitrate(float durationSeconds, int maxSizeMB) {
    const long long totalBits = static_cast<long long>(maxSizeMB) * 8 * 1024 * 1024;
    const int audioBitrate = 128;  // kbps

    int videoBitrate = (totalBits / durationSeconds / 1000) - audioBitrate;

    // Minimum 100kbps
    if (videoBitrate < 100) videoBitrate = 100;

    printf("[VideoExporter] Duration: %.2fs, MaxSize: %dMB, Bitrate: %dkbps\n",
           durationSeconds, maxSizeMB, videoBitrate);

    return videoBitrate;
}

void VideoExporter::KillFFmpegProcess() {
    m_shouldCancel = true;

    if (!m_outputPath.empty()) {
        const std::string killCmd = "pkill -9 -f \"" + m_outputPath + "\" 2>/dev/null";
        system(killCmd.c_str());
        printf("[VideoExporter] Killed FFmpeg processes for: %s\n", m_outputPath.c_str());
    }

    if (m_pipe != nullptr) {
        pclose(m_pipe);
        m_pipe = nullptr;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}