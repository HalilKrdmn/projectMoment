#include "gui/utils/FormatUtils.h"
#include <sstream>
#include <iomanip>
#include <chrono>

std::string FormatUtils::FormatDuration(double seconds) {
    if (seconds < 0) {
        return "00:00";
    }
    
    int hours = static_cast<int>(seconds / 3600);
    int minutes = static_cast<int>((seconds - hours * 3600) / 60);
    int secs = static_cast<int>(seconds - hours * 3600 - minutes * 60);
    
    std::ostringstream oss;
    
    if (hours > 0) {
        oss << hours << ":"
            << std::setfill('0') << std::setw(2) << minutes << ":"
            << std::setfill('0') << std::setw(2) << secs;
    } else {
        oss << minutes << ":"
            << std::setfill('0') << std::setw(2) << secs;
    }
    
    return oss.str();
}

std::string FormatUtils::FormatDate(long long timestampMs) {
    if (timestampMs == 0) {
        return "Unknown";
    }
    
    auto timePoint = std::chrono::system_clock::time_point(
        std::chrono::milliseconds(timestampMs)
    );
    
    auto time_t = std::chrono::system_clock::to_time_t(timePoint);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M");
    
    return oss.str();
}

std::string FormatUtils::FormatFileSize(long long bytes) {
    if (bytes < 0) {
        return "0 B";
    }
    
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    
    return oss.str();
}

std::string FormatUtils::FormatResolution(int width, int height) {
    std::ostringstream oss;
    oss << width << "x" << height;

    if (width == 1920 && height == 1080) {
        oss << " (FHD)";
    } else if (width == 2560 && height == 1440) {
        oss << " (2K)";
    } else if (width == 3840 && height == 2160) {
        oss << " (4K)";
    } else if (width == 1280 && height == 720) {
        oss << " (HD)";
    } else if (width == 7680 && height == 4320) {
        oss << " (8K)";
    }
    
    return oss.str();
}

std::string FormatUtils::FormatFrameRate(int fps) {
    std::ostringstream oss;
    oss << fps << " fps";
    return oss.str();
}