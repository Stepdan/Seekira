#include "image_utils.hpp"
#include "utils.hpp"

#include <core/log/log.hpp>
#include <core/base/utils/find_pair.hpp>
#include <core/exception/assert.hpp>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace {

/* clang-format off */
const std::pair<step::video::PixFmt, AVPixelFormat> g_avformat_to_pixfmt[] = {
    { step::video::PixFmt::GRAY, AV_PIX_FMT_GRAY8 },
    { step::video::PixFmt::BGR , AV_PIX_FMT_BGR24 },
    { step::video::PixFmt::RGB , AV_PIX_FMT_RGB24 },
    { step::video::PixFmt::RGBA, AV_PIX_FMT_RGBA  },
    { step::video::PixFmt::BGRA, AV_PIX_FMT_BGRA  },
};
/* clang-format on */

}  // namespace

namespace step::video::ff {

PixFmt avformat_to_pix_fmt(AVPixelFormat fmt)
{
    if (auto it = step::utils::find_pair_iterator_by_second(g_avformat_to_pixfmt, fmt);
        it != std::cend(g_avformat_to_pixfmt))
    {
        return it->first;
    }
    else
    {
        STEP_THROW_RUNTIME("Invalid AVPixelFormat {}!", fmt);
    }
}

AVPixelFormat pix_fmt_to_avformat(PixFmt fmt)
{
    if (auto it = step::utils::find_pair_iterator_by_first(g_avformat_to_pixfmt, fmt);
        it != std::cend(g_avformat_to_pixfmt))
    {
        return it->second;
    }
    else
    {
        STEP_THROW_RUNTIME("Invalid PixFmt {}!", fmt);
    }
}

Frame avframe_to_frame(const AVFrame* avframe)
{
    STEP_ASSERT(avframe, "Invalid avframe!");

    const auto width = static_cast<size_t>(avframe->width);
    const auto height = static_cast<size_t>(avframe->height);
    STEP_ASSERT(avframe->linesize[0] > 0, "Avframe linesize < 0!");
    const auto stride = static_cast<size_t>(avframe->linesize[0]);
    const auto pix_fmt = avformat_to_pix_fmt(static_cast<AVPixelFormat>(avframe->format));

    auto frame = Frame::create_deep({width, height}, stride, pix_fmt, const_cast<uint8_t*>(avframe->data[0]));

    return frame;
}

AVFrame* allocate_avframe(AVPixelFormat fmt, int width, int height, int stride)
{
    STEP_ASSERT(fmt == AVPixelFormat::AV_PIX_FMT_BGR24, "Only AV_PIX_FMT_BGR24 supported for allocate_avframe");

    AVFrame* frame;
    frame = av_frame_alloc();
    frame->width = width;
    frame->height = height;
    frame->format = fmt;

    // TODO Can be different for planar types
    frame->linesize[0] = stride;

    auto res = av_frame_get_buffer(frame, 0);
    if(res < 0)
    {
        STEP_LOG(L_ERROR, "Can't allocate avframe! Error {}", av_make_error(res));
        av_frame_free(&frame);
        return nullptr;
    }


    return frame;
}

}  // namespace step::video::ff