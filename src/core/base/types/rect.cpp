#include "rect.hpp"

#include "config_fields.hpp"

#include <limits>

namespace step {

bool Rect::operator==(const Rect& rhs) const noexcept
{
    /* clang-format off */
    return true
        && p0 == rhs.p0
        && p1 == rhs.p1
    ;
    /* clang-format on */
}

bool Rect::is_valid() const noexcept { return *this != Rect(); }

void Rect::serialize(ObjectPtrJSON& container)
{
    auto p0_json = json::make_object_json_ptr();
    auto p1_json = json::make_object_json_ptr();

    p0.serialize(p0_json);
    p1.serialize(p1_json);
    json::set(container, CFG_FLD::POINT0, p0);
    json::set(container, CFG_FLD::POINT1, p1);
}

void Rect::deserialize(const ObjectPtrJSON& container)
{
    auto p0_json = json::get_object(container, CFG_FLD::POINT0);
    auto p1_json = json::get_object(container, CFG_FLD::POINT1);
    p0.deserialize(p0_json);
    p1.deserialize(p1_json);
}

}  // namespace step