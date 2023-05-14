#pragma once

#include <fmt/format.h>

#include <cstddef>

namespace step::video {

class FrameSize
{
public:
    FrameSize() : width(0), height(0) {}
    FrameSize(size_t w, size_t h) : width(w), height(h){};

    bool operator==(const FrameSize& rhs) const { return width == rhs.width && height == rhs.height; }
    bool operator!=(const FrameSize& rhs) const { return !(*this == rhs); }

    FrameSize operator*(double scale_factor) const
    {
        return FrameSize(static_cast<size_t>(width * scale_factor + 0.5),
                         static_cast<size_t>(height * scale_factor + 0.5));
    }
    FrameSize& operator*=(double scale_factor)
    {
        *this = *this * scale_factor;
        return *this;
    }

    size_t width, height;
};

}  // namespace step::video

template <>
struct fmt::formatter<step::video::FrameSize> : fmt::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const step::video::FrameSize& size, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "width: {}, height: {}", size.width, size.height);
    }
};