#pragma once

extern "C" {
#include <libavformat/format.h>
#include <libavcodec/packet.h>
#include <libavutil/error.h>
}

#include <cstring>

namespace step::video::ff {

constexpr uint16_t MAX_STREAMS{1024};
constexpr AVRational TIME_BASE_Q{1, AV_TIME_BASE};

// https://github.com/bmewj/video-app/blob/master/src/video_reader.cpp
// av_err2str returns a temporary array. This doesn't work in gcc.
// This function can be used as a replacement for av_err2str.
static const char* av_make_error(int errnum)
{
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

AVPacket* create_packet(size_t size);
AVPacket* copy_packet(const AVPacket* src);
void release_packet(AVPacket** packet);

int read_frame_fixed(AVFormatContext* ctx, AVPacket* pkt);

int64_t stream_to_global(int64_t stream_time, const AVRational& stream_time_base);
int64_t global_to_stream(int64_t global_time, const AVRational& stream_time_base);

}  // namespace step::video::ff