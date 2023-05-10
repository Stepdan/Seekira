#include "ff_image_utils.hpp"

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

    uint8_t* buffer = nullptr;
    auto buf_size = av_image_get_buffer_size(static_cast<AVPixelFormat>(avframe->format), width, height, 1);

    auto copy_ret =
        av_image_copy_to_buffer(buffer, buf_size, (const uint8_t* const*)avframe->data, (const int*)avframe->linesize,
                                static_cast<AVPixelFormat>(avframe->format), avframe->width, avframe->height, 1);
    STEP_ASSERT(copy_ret < 0, "Invalid avframe buffer copy!");

    auto frame = Frame::create_deep({width, height}, stride, pix_fmt, buffer);

    if (buffer)
        av_freep(&buffer);

    return frame;
}

}  // namespace step::video::ff