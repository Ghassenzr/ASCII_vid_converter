#include <iostream>
#include "montage.h"
int main()
{
    VideoInfo infotest = Analyserer("The_Unity_Tutorial_For_Complete_Beginners_merged.mp4");
    printf("Hello, ASCII Vid Converter!\n");
    printf("Video Info:\n");
    printf("Frames: %d\n", infotest.frames);
    printf("Duration (seconds): %.2f\n", infotest.duration_seconds);
    printf("FPS: %.2f\n", infotest.fps);
    printf("Width: %d\n", infotest.width);
    printf("Height: %d\n", infotest.height);
    printf("Codec Name: %s\n", infotest.codec_name.c_str());
    printf("Pixel Format: %s\n", infotest.pixel_format.c_str());
    printf("Has Video: %s\n", infotest.has_video ? "Yes" : "No");
    printf("Has Audio: %s\n", infotest.has_audio ? "Yes" : "No");
    return 0;
}
