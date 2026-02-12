#pragma once

#include <string>
#include <functional>

class VideoLibrary;
class LoadingState;

class LibraryLoader {
public:
    using ProgressCallback = std::function<void(const std::string&, float)>;

    static void Run(VideoLibrary* library, 
                    const std::string& libraryPath,
                    const ProgressCallback &onProgress = nullptr);
};