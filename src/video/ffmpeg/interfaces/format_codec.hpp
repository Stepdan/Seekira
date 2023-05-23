#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <fmt/format.h>

namespace step::video::ff {

struct FormatCodec
{
    AVCodecParameters* codec_par;
    AVRational fps;
    bool image_flag{false};
};

}  // namespace step::video::ff

template <>
struct fmt::formatter<step::video::ff::FormatCodec> : fmt::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const step::video::ff::FormatCodec& fc, FormatContext& ctx)
    {
        return fmt::format_to(
            ctx.out(), "codec_id: {}, codec_tag: {}, width: {}, height: {}, FPS (num) {}, FPS (den) {}, image_flag: {}",
            fc.codec_par->codec_id, fc.codec_par->codec_tag, fc.codec_par->width, fc.codec_par->height, fc.fps.num,
            fc.fps.den, fc.image_flag);
    }
};
