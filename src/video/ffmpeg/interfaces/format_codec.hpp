#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <fmt/format.h>

namespace step::video::ff {

struct FormatCodec
{
    AVCodecID codec_id;
    uint32_t codec_tag;
    size_t width;
    size_t height;
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
            fc.codec_id, fc.codec_tag, fc.width, fc.height, fc.fps.num, fc.fps.den, fc.image_flag);
    }
};
