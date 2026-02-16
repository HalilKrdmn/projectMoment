#pragma once

#include "core/media/VideoExporter.h"

#include <memory>


class EditingScreen;

enum class ExportQuality {
    SIZE_10MB,
    SIZE_50MB,
    SIZE_100MB,
    ORIGINAL
};

class ExportWidget {
public:
    ExportWidget();

    void Draw(const EditingScreen* parent);

    void Reset();

private:
    static void DrawDimBackground();

    static std::string GenerateDefaultFilename(const std::string& inputFilename);
    void AddLog(const std::string& message);

    std::unique_ptr<VideoExporter> m_exporter;
    char m_outputFilename[256];
    ExportQuality m_selectedQuality = ExportQuality::ORIGINAL;

    std::vector<std::string> m_exportLogs;
    float m_lastProgress = 0.0f;
    bool m_initialized = false;
};