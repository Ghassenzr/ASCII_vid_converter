extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

#include <iostream>

struct VideoInfo
{
    AVCodecParameters *codecpar;
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

VideoInfo Analyserer(const char *video_path)
{
    AVFormatContext *format_ctx = nullptr;
    VideoInfo info = {};
    if (avformat_open_input(&format_ctx, video_path, nullptr, nullptr) < 0)
    {
        std::cerr << "Couldn't open file: " << video_path << std::endl;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0)
    {
        std::cerr << "Couldn't find stream information." << std::endl;
        avformat_close_input(&format_ctx);
    }

    int video_stream_index = -1;
    int audio_stream_index = -1;

    for (unsigned int i = 0; i < format_ctx->nb_streams; i++)
    {
        AVStream *stream = format_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;

        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1)
            video_stream_index = i;
        else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1)
            audio_stream_index = i;
    }
    info.has_video = (video_stream_index != -1);
    info.has_audio = (audio_stream_index != -1);
    AVStream *video_stream = format_ctx->streams[video_stream_index];
    int64_t duration_microseconds = format_ctx->duration;
    AVRational time_base = video_stream->time_base;
    double duration_seconds = duration_microseconds * av_q2d(time_base);
    info.duration_seconds = duration_seconds;
    info.frames = static_cast<int>(video_stream->nb_frames);
    AVRational frame_rate = video_stream->r_frame_rate;
    double fps = frame_rate.num / static_cast<double>(frame_rate.den);
    info.fps = fps;
    AVCodecParameters *codecpar = video_stream->codecpar;
    info.codecpar = codecpar;
    int width = codecpar->width;
    info.width = width;
    int height = codecpar->height;
    info.height = height;

    AVPixelFormat pixel_format = static_cast<AVPixelFormat>(codecpar->format);
    info.pixel_format = av_get_pix_fmt_name(pixel_format);
    AVCodecID codec_id = codecpar->codec_id;
    info.codec_name = avcodec_get_name(codec_id);

    avformat_close_input(&format_ctx);
    return info;
}
void Videodecoder(const char *video_path)
{
    VideoInfo info = Analyserer(video_path);
    const AVCodec *codec = avcodec_find_decoder(info.codecpar->codec_id);
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, info.codecpar);
    avcodec_open2(codec_ctx, codec, NULL);
    const AVPacket *avpkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    //////////////////////////////////////////////////////////////////////////
    AVFormatContext *format_ctx = nullptr;
    AVCodecContext *codec_ctx = avcodec_alloc_context3(nullptr);
    avformat_open_input(&format_ctx, video_path, nullptr, nullptr);
    while (av_read_frame(format_ctx, avpkt) >= 0)
    {
        if (packet->stream_index == video_stream_index)
        {

            // Send packet to decoder
            avcodec_send_packet(codec_ctx, packet);

            // Receive all available frames
            while (avcodec_receive_frame(codec_ctx, frame) == 0)
            {

                // Process frame here
                // frame->data[0] contains pixel data
                // frame->linesize[0] is row stride
                // frame->width, frame->height

                av_frame_unref(frame); // Release frame data
            }
        }

        av_packet_unref(packet); // Release packet data
    }

    // Flush decoder (get remaining frames)
    avcodec_send_packet(codec_ctx, NULL);
    while (avcodec_receive_frame(codec_ctx, frame) == 0)
    {
        // Process final frames
        av_frame_unref(frame);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    ///////////////////////////////////////////////////////////////////////////
}