#include "Point2D.hpp"

#include "config_fields.hpp"

#include <limits>

namespace step {

Point2D::Point2D() : x(std::numeric_limits<int>::min()), y(std::numeric_limits<int>::min()) {}

bool Point2D::operator==(const Point2D& rhs) const noexcept
{
    /* clang-format off */
    return true
        && x == rhs.x
        && y == rhs.y
    ;
    /* clang-format on */
}

bool Point2D::is_valid() const noexcept { return *this != Point2D(); }

void Point2D::serialize(ObjectPtrJSON& container)
{
    json::set(container, CFG_FLD::X, x);
    json::set(container, CFG_FLD::Y, y);
}

void Point2D::deserialize(const ObjectPtrJSON& container)
{
    x = json::get<int>(container, CFG_FLD::X);
    y = json::get<int>(container, CFG_FLD::Y);
}

}  // namespace step