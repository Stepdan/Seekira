#pragma once

#include <core/base/utils/string_utils.hpp>

#include <fmt/format.h>

namespace step::video {

/*
    Pixel format (Colorspace)
*/
enum class PixFmt : int
{
    Undefined = 0,
    GRAY,
    RGB,
    RGBA,
    BGR,
    BGRA,

    // ATTENTION always should be the last one
    EndOf
};

}  // namespace step::video

namespace step::video::utils {

size_t get_bpp(PixFmt fmt);
size_t get_channels_count(PixFmt fmt);
bool has_alpha(PixFmt fmt);

}  // namespace step::video::utils

template <>
struct fmt::formatter<step::video::PixFmt> : fmt::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const step::video::PixFmt& pix_fmt, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{}", step::utils::to_string(pix_fmt));
    }
};
