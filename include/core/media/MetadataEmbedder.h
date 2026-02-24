#pragma once

#include "core/VideoInfo.h"


#include <string>
#include <map>

class MetadataEmbedder{
public:
    static bool WriteMetadataToVideo(const std::string& videoPath, const VideoInfo& info);
    static bool ReadMetadataFromVideo(const std::string& videoPath, VideoInfo& info);

    static bool HasEmbeddedMetadata(const std::string& videoPath);

    static bool AddCustomTag(const std::string& videoPath,
                             const std::string& key,
                             const std::string& value);
    
private:
    static std::map<std::string, std::string> VideoInfoToTags(const VideoInfo& info);
    static void TagsToVideoInfo(const std::map<std::string, std::string>& tags, VideoInfo& info);


    static bool CopyVideoWithNewMetadata(
        const std::string& inputPath,
        const std::string& outputPath,
        const std::map<std::string, std::string>& metadata
    );
};
