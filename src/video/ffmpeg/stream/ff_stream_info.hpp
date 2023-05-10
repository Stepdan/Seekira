#pragma once

#include <video/frame/interfaces/pixel_format.hpp>
#include <video/frame/interfaces/frame_size.hpp>

#include <fmt/format.h>

namespace step::video::ff {

class FFStreamInfo
{
public:
    FFStreamInfo();
    FFStreamInfo(const std::string& filename, const std::string& format, const std::string& codec, PixFmt pix_fmt,
                 const FrameSize& frame_size, double fps, int64_t duration)
        : m_filename(filename)
        , m_format(format)
        , m_codec(codec)
        , m_pix_fmt(pix_fmt)
        , m_frame_size(frame_size)
        , m_fps(fps)
        , m_duration(duration)
    {
    }

    const std::string& filename() const noexcept { return m_filename; }
    const std::string& format() const noexcept { return m_format; }
    const std::string& codec() const noexcept { return m_codec; }
    PixFmt pix_fmt() const noexcept { return m_pix_fmt; }
    const FrameSize& frame_size() const noexcept { return m_frame_size; }
    double fps() const noexcept { return m_fps; }
    int64_t duration() const noexcept { return m_duration; }

private:
    std::string m_filename;
    std::string m_format;
    std::string m_codec;
    PixFmt m_pix_fmt{PixFmt::Undefined};
    FrameSize m_frame_size;
    double m_fps;
    int64_t m_duration;
};

}  // namespace step::video::ff

template <>
struct fmt::formatter<step::video::ff::FFStreamInfo> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::video::ff::FFStreamInfo& info, FormatContext& ctx)
    {
        return format_to(ctx.out(), "filename {}, format {}, codec {}, pix_fmt: {}, size: [{}], fps {}, duration {}",
                         info.filename(), info.format(), info.codec(), info.pix_fmt(), info.frame_size(), info.fps(),
                         info.duration());
    }
};
