#pragma once

extern "C" {
#include <libavutil/error.h>
}

#include <cstring>

namespace step::video::ff {

// https://github.com/bmewj/video-app/blob/master/src/video_reader.cpp
// av_err2str returns a temporary array. This doesn't work in gcc.
// This function can be used as a replacement for av_err2str.
static const char* av_make_error(int errnum)
{
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

}  // namespace step::video::ff