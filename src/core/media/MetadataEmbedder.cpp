#include "core/media/MetadataEmbedder.h"
#include <filesystem>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace fs = std::filesystem;

bool MetadataEmbedder::WriteMetadataToVideo(
    const std::string& videoPath, 
    const VideoInfo& info) {

    const auto metadata = VideoInfoToTags(info);

    if (const std::string tempPath = videoPath + ".temp.mp4"; CopyVideoWithNewMetadata(videoPath, tempPath, metadata)) {
        fs::remove(videoPath);
        fs::rename(tempPath, videoPath);
        return true;
    } else {
        if (fs::exists(tempPath)) {
            fs::remove(tempPath);
        }
        return false;
    }
}

bool MetadataEmbedder::ReadMetadataFromVideo(
    const std::string& videoPath, 
    VideoInfo& info) {
    
    AVFormatContext* formatCtx = nullptr;
    
    if (avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr) != 0) {
        return false;
    }
    
    avformat_find_stream_info(formatCtx, nullptr);

    const AVDictionaryEntry* tag = nullptr;
    std::map<std::string, std::string> tags;
    
    while ((tag = av_dict_get(formatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        tags[tag->key] = tag->value;
    }

    for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            const AVStream* stream = formatCtx->streams[i];
            
            info.fileResolutionWidth = stream->codecpar->width;
            info.fileResolutionHeight = stream->codecpar->height;
            info.resolutionWidth = info.fileResolutionWidth;
            info.resolutionHeight = info.fileResolutionHeight;
            info.frameRate = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
            
            if (stream->duration != AV_NOPTS_VALUE) {
                info.durationSec = stream->duration * av_q2d(stream->time_base);
            }
            break;
        }
    }
    
    avformat_close_input(&formatCtx);

    TagsToVideoInfo(tags, info);
    
    info.filePath = videoPath;
    info.filePathString = videoPath;
    
    return !tags.empty();
}

bool MetadataEmbedder::HasEmbeddedMetadata(const std::string& videoPath) {
    AVFormatContext* formatCtx = nullptr;
    
    if (avformat_open_input(&formatCtx, videoPath.c_str(), nullptr, nullptr) != 0) {
        return false;
    }

    const AVDictionaryEntry* tag = av_dict_get(formatCtx->metadata, "app_version", nullptr, 0);
    const bool hasMetadata = (tag != nullptr);
    
    avformat_close_input(&formatCtx);
    return hasMetadata;
}

bool MetadataEmbedder::CopyVideoWithNewMetadata(
    const std::string& inputPath,
    const std::string& outputPath,
    const std::map<std::string, std::string>& metadata) {
    
    AVFormatContext* inputCtx = nullptr;
    AVFormatContext* outputCtx = nullptr;

    if (avformat_open_input(&inputCtx, inputPath.c_str(), nullptr, nullptr) < 0) {
        return false;
    }
    
    avformat_find_stream_info(inputCtx, nullptr);

    avformat_alloc_output_context2(&outputCtx, nullptr, nullptr, outputPath.c_str());
    if (!outputCtx) {
        avformat_close_input(&inputCtx);
        return false;
    }

    for (unsigned int i = 0; i < inputCtx->nb_streams; i++) {
        const AVStream* inStream = inputCtx->streams[i];
        const AVStream* outStream = avformat_new_stream(outputCtx, nullptr);
        
        if (!outStream) {
            avformat_close_input(&inputCtx);
            avformat_free_context(outputCtx);
            return false;
        }

        avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
        outStream->codecpar->codec_tag = 0;
    }

    for (const auto&[fst, snd] : metadata) {
        av_dict_set(&outputCtx->metadata, fst.c_str(), snd.c_str(), 0);
    }

    if (!(outputCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outputCtx->pb, outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
            avformat_close_input(&inputCtx);
            avformat_free_context(outputCtx);
            return false;
        }
    }

    avformat_write_header(outputCtx, nullptr);

    AVPacket packet;
    while (av_read_frame(inputCtx, &packet) >= 0) {
        const AVStream* inStream = inputCtx->streams[packet.stream_index];
        const AVStream* outStream = outputCtx->streams[packet.stream_index];

        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, 
                                       outStream->time_base, 
                                       static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, 
                                       outStream->time_base, 
                                       static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, inStream->time_base, 
                                        outStream->time_base);
        packet.pos = -1;

        av_interleaved_write_frame(outputCtx, &packet);
        av_packet_unref(&packet);
    }

    av_write_trailer(outputCtx);

    if (!(outputCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outputCtx->pb);
    }
    
    avformat_close_input(&inputCtx);
    avformat_free_context(outputCtx);
    
    return true;
}

std::map<std::string, std::string> MetadataEmbedder::VideoInfoToTags(const VideoInfo& info) {
    std::map<std::string, std::string> tags;

    tags["title"] = info.name;
    tags["comment"] = info.description;

    tags["app_version"] = info.appVersion.empty() ? "0.0.1" : info.appVersion;
    tags["clip_start"] = std::to_string(info.clipStartPoint);
    tags["clip_end"] = std::to_string(info.clipEndPoint);
    tags["tags"] = info.tagsStorage;
    tags["recording_time"] = std::to_string(info.recordingTimeMs);
    tags["last_edit_time"] = std::to_string(info.lastEditTimeMs);
    tags["thumbnail_path"] = info.thumbnailPath;

    tags["audio_codec"] = info.audioCodec;
    tags["audio_bitrate"] = std::to_string(info.audioBitrate);
    tags["audio_sample_rate"] = std::to_string(info.audioSampleRate);
    tags["audio_tracks"] = info.audioTrackNamesStr;
    
    return tags;
}

void MetadataEmbedder::TagsToVideoInfo(
    const std::map<std::string, std::string>& tags, 
    VideoInfo& info) {

    if (tags.contains("title")) info.name = tags.at("title");
    if (tags.contains("comment")) info.description = tags.at("comment");
    
    // Custom tags
    if (tags.contains("app_version")) info.appVersion = tags.at("app_version");
    if (tags.contains("clip_start")) info.clipStartPoint = std::stod(tags.at("clip_start"));
    if (tags.contains("clip_end")) info.clipEndPoint = std::stod(tags.at("clip_end"));
    if (tags.contains("tags")) info.tagsStorage = tags.at("tags");
    if (tags.contains("recording_time")) info.recordingTimeMs = std::stoll(tags.at("recording_time"));
    if (tags.contains("last_edit_time")) info.lastEditTimeMs = std::stoll(tags.at("last_edit_time"));
    if (tags.contains("thumbnail_path")) info.thumbnailPath = tags.at("thumbnail_path");
    
    // Sound info
    if (tags.contains("audio_codec")) info.audioCodec = tags.at("audio_codec");
    if (tags.contains("audio_bitrate")) info.audioBitrate = std::stoi(tags.at("audio_bitrate"));
    if (tags.contains("audio_sample_rate")) info.audioSampleRate = std::stoi(tags.at("audio_sample_rate"));
    if (tags.contains("audio_tracks")) info.audioTrackNamesStr = tags.at("audio_tracks");
}

bool MetadataEmbedder::AddCustomTag(
    const std::string& videoPath,
    const std::string& key,
    const std::string& value) {

    VideoInfo info;
    ReadMetadataFromVideo(videoPath, info);

    auto tags = VideoInfoToTags(info);
    tags[key] = value;

    if (const std::string tempPath = videoPath + ".temp.mp4"; CopyVideoWithNewMetadata(videoPath, tempPath, tags)) {
        fs::remove(videoPath);
        fs::rename(tempPath, videoPath);
        return true;
    }
    
    return false;
}