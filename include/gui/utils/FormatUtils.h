#pragma once
#include <string>

class FormatUtils {
public:
    static std::string FormatDuration(double seconds);

    static std::string FormatDate(long long timestampMs);

    static std::string FormatFileSize(long long bytes);

    static std::string FormatResolution(int width, int height);

    static std::string FormatFrameRate(int fps);
    
private:
    FormatUtils() = delete;  // Static-only class
};