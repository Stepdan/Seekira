#pragma once

#include <core/base/interfaces/serializable.hpp>

#include <fmt/format.h>

namespace step {

struct ColorRGB : public ISerializable
{
    int red, green, blue;

    ColorRGB();
    ColorRGB(int r, int g, int b) : red(r), green(g), blue(b) {}

    bool operator==(const ColorRGB& rhs) const noexcept;
    bool operator!=(const ColorRGB& rhs) const noexcept { return !(*this == rhs); }

    bool is_valid() const noexcept;

    void serialize(ObjectPtrJSON&) override;
    void deserialize(const ObjectPtrJSON&) override;
};

class COLOR_RGB
{
public:
    static const ColorRGB Red;
    static const ColorRGB Green;
    static const ColorRGB Blue;
    static const ColorRGB Black;
    static const ColorRGB White;
};

}  // namespace step

template <>
struct fmt::formatter<step::ColorRGB> : fmt::formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::ColorRGB& color, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "(r: {}, g: {}, b: {})", color.red, color.green, color.blue);
    }
};