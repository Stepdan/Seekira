#pragma once

#include "point2d.hpp"

namespace step {

struct Rect : public ISerializable
{
    Point2D p0, p1;

    Rect() = default;
    Rect(int _x0, int _y0, int _x1, int _y1) : p0(_x0, _y0), p1(_x1, _y1) {}
    Rect(const Point2D& _p0, const Point2D& _p1) : p0(_p0), p1(_p1) {}

    bool operator==(const Rect& rhs) const noexcept;
    bool operator!=(const Rect& rhs) const noexcept { return !(*this == rhs); }

    bool is_valid() const noexcept;

    int length() const noexcept { return p1.x - p0.x; }
    int height() const noexcept { return p1.y - p0.y; }

    void serialize(ObjectPtrJSON&) override;
    void deserialize(const ObjectPtrJSON&) override;
};

}  // namespace step

template <>
struct fmt::formatter<step::Rect> : fmt::formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::Rect& rect, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "{}x{}", rect.p0, rect.p1);
    }
};