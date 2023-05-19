#pragma once

#include <video/frame/interfaces/frame.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace step::video::ff {

PixFmt avformat_to_pix_fmt(AVPixelFormat fmt);
AVPixelFormat pix_fmt_to_avformat(PixFmt fmt);

Frame avframe_to_frame(const AVFrame* avframe);

}  // namespace step::video::ff