#pragma once

#include <core/base/interfaces/serializable.hpp>

#include <fmt/format.h>

#include <cstddef>

namespace step::video {

struct CameraResolution : public ISerializable
{
    CameraResolution() = default;
    CameraResolution(size_t w, size_t h) : width(w), height(h) {}

    bool operator==(const CameraResolution& rhs) const noexcept;
    bool operator!=(const CameraResolution& rhs) const noexcept { return !(*this == rhs); }

    void serialize(ObjectPtrJSON&);
    void deserialize(const ObjectPtrJSON&);

    size_t width{0};
    size_t height{0};
};

}  // namespace step::video

template <>
struct fmt::formatter<step::video::CameraResolution> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::video::CameraResolution& size, FormatContext& ctx)
    {
        return format_to(ctx.out(), "width: {}, height: {}", size.width, size.height);
    }
};
