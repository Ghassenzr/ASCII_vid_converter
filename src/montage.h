#ifndef MONTAGE_H
#define MONTAGE_H

#include <string>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

struct VideoInfo
{
    int frames;
    double duration_seconds;
    double fps;
    int width;
    int height;
    std::string codec_name;
    std::string pixel_format;
    bool has_video;
    bool has_audio;
};

VideoInfo Analyserer(const char *video_path);

#endif