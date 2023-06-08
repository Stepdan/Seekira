#pragma once

#include <core/base/interfaces/serializable.hpp>

#include <fmt/format.h>

namespace step {

struct Point2D : public ISerializable
{
    int x, y;

    Point2D();
    Point2D(int _x, int _y) : x(_x), y(_y) {}

    bool operator==(const Point2D& rhs) const noexcept;
    bool operator!=(const Point2D& rhs) const noexcept { return !(*this == rhs); }

    bool is_valid() const noexcept;

    void serialize(ObjectPtrJSON&) override;
    void deserialize(const ObjectPtrJSON&) override;
};

}  // namespace step

template <>
struct fmt::formatter<step::Point2D> : fmt::formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::Point2D& pnt, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "({}, {})", pnt.x, pnt.y);
    }
};