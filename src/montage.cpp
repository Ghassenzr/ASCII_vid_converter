extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

#include <iostream>

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

VideoInfo Analyserer(const char *video_path)
{
    AVFormatContext *format_ctx = nullptr;
    VideoInfo info = {};
    if (avformat_open_input(&format_ctx, video_path, nullptr, nullptr) < 0)
    {
        std::cerr << "Couldn't open file: " << video_path << std::endl;
        return info;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0)
    {
        std::cerr << "Couldn't find stream information." << std::endl;
        avformat_close_input(&format_ctx);
        return info;
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
    if (video_stream_index == -1)
    {
        std::cerr << "No video stream found." << std::endl;
        avformat_close_input(&format_ctx);
        return info;
    }
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

void VideoDecoder(const char *video_path)
{
    // Open file
    AVFormatContext *format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, video_path, nullptr, nullptr) < 0)
    {
        std::cerr << "Cannot open file\n";
        return;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0)
    {
        std::cerr << "Cannot find stream info\n";
        avformat_close_input(&format_ctx);
        return;
    }

    // Find streams
    int video_stream_index = -1;
    int audio_stream_index = -1;

    for (unsigned int i = 0; i < format_ctx->nb_streams; i++)
    {
        AVCodecParameters *codecpar = format_ctx->streams[i]->codecpar;

        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1)
            video_stream_index = i;
        else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1)
            audio_stream_index = i;
    }

    // Check if video exists
    if (video_stream_index == -1)
    {
        std::cerr << "No video stream found\n";
        avformat_close_input(&format_ctx);
        return;
    }

    // Setup video decoder
    AVStream *video_stream = format_ctx->streams[video_stream_index];
    AVCodecParameters *codecpar = video_stream->codecpar;

    const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec)
    {
        std::cerr << "Decoder not found\n";
        avformat_close_input(&format_ctx);
        return;
    }

    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        std::cerr << "Cannot allocate codec context\n";
        avformat_close_input(&format_ctx);
        return;
    }

    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0)
    {
        std::cerr << "Cannot copy codec parameters\n";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
    {
        std::cerr << "Cannot open decoder\n";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return;
    }

    // Allocate packet and frame
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    if (!packet || !frame)
    {
        std::cerr << "Cannot allocate packet/frame\n";
        av_packet_free(&packet);
        av_frame_free(&frame);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return; // ← Fixed: Added return
    }

    // Setup audio output
    AVFormatContext *audio_output_ctx = nullptr;

    if (audio_stream_index != -1)
    {
        if (avformat_alloc_output_context2(&audio_output_ctx, nullptr, nullptr, "audio.aac") >= 0)
        {

            AVStream *out_audio_stream = avformat_new_stream(audio_output_ctx, nullptr);

            if (out_audio_stream)
            {
                avcodec_parameters_copy(out_audio_stream->codecpar,
                                        format_ctx->streams[audio_stream_index]->codecpar);
                out_audio_stream->codecpar->codec_tag = 0;

                if (avio_open(&audio_output_ctx->pb, "audio.aac", AVIO_FLAG_WRITE) >= 0)
                {
                    if (avformat_write_header(audio_output_ctx, nullptr) >= 0)
                    {
                        std::cout << "Audio extraction enabled\n";
                    }
                    else
                    {
                        std::cerr << "Cannot write audio header\n";
                        avio_closep(&audio_output_ctx->pb);
                        avformat_free_context(audio_output_ctx);
                        audio_output_ctx = nullptr;
                    }
                }
                else
                {
                    std::cerr << "Cannot open audio.aac\n";
                    avformat_free_context(audio_output_ctx);
                    audio_output_ctx = nullptr;
                }
            }
            else
            {
                std::cerr << "Cannot create audio stream\n";
                avformat_free_context(audio_output_ctx);
                audio_output_ctx = nullptr;
            }
        }
        else
        {
            std::cerr << "Cannot create audio output\n";
        }
    }

    // Main decode loop
    int frame_count = 0;

    while (av_read_frame(format_ctx, packet) >= 0)
    {

        // Handle video packets
        if (packet->stream_index == video_stream_index)
        {

            if (avcodec_send_packet(codec_ctx, packet) < 0)
            {
                std::cerr << "Error sending packet\n";
                av_packet_unref(packet);
                break;
            }

            while (avcodec_receive_frame(codec_ctx, frame) == 0)
            {
                printf("Decoded frame %d (size: %dx%d)\n",
                       frame_count++, frame->width, frame->height);

                // TODO: Save frame as image or convert to ASCII

                av_frame_unref(frame);
            }
        }
        // Handle audio packets
        else if (packet->stream_index == audio_stream_index && audio_output_ctx)
        {

            av_packet_rescale_ts(packet,
                                 format_ctx->streams[audio_stream_index]->time_base,
                                 audio_output_ctx->streams[0]->time_base);
            packet->stream_index = 0;

            if (av_interleaved_write_frame(audio_output_ctx, packet) < 0)
            {
                std::cerr << "Error writing audio packet\n";
            }
        }

        av_packet_unref(packet);
    }

    // Finalize audio
    if (audio_output_ctx)
    {
        av_write_trailer(audio_output_ctx);
        avio_closep(&audio_output_ctx->pb);
        avformat_free_context(audio_output_ctx);
        std::cout << "Audio saved to audio.aac\n";
    }

    // Flush decoder
    avcodec_send_packet(codec_ctx, nullptr);

    while (avcodec_receive_frame(codec_ctx, frame) == 0)
    {
        printf("Flushed frame %d\n", frame_count++);
        av_frame_unref(frame);
    }

    // Cleanup
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    printf("Total frames decoded: %d\n", frame_count);
}
