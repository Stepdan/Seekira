#include "color.hpp"

#include "config_fields.hpp"

#include <limits>

namespace step {

ColorRGB::ColorRGB()
    : red(std::numeric_limits<int>::min())
    , green(std::numeric_limits<int>::min())
    , blue(std::numeric_limits<int>::min())
{
}

bool ColorRGB::operator==(const ColorRGB& rhs) const noexcept
{
    /* clang-format off */
    return true
        && red == rhs.red
        && green == rhs.green
        && blue == rhs.blue
    ;
    /* clang-format on */
}

bool ColorRGB::is_valid() const noexcept { return *this != ColorRGB(); }

void ColorRGB::serialize(ObjectPtrJSON& container)
{
    json::set(container, CFG_FLD::RED, red);
    json::set(container, CFG_FLD::GREEN, green);
    json::set(container, CFG_FLD::BLUE, blue);
}

void ColorRGB::deserialize(const ObjectPtrJSON& container)
{
    red = json::get<int>(container, CFG_FLD::RED);
    green = json::get<int>(container, CFG_FLD::GREEN);
    blue = json::get<int>(container, CFG_FLD::BLUE);
}

}  // namespace step

namespace step {
/* clang-format off */
const ColorRGB COLOR_RGB::Red   = ColorRGB(255  , 0     , 0     );
const ColorRGB COLOR_RGB::Green = ColorRGB(0    , 255   , 0     );
const ColorRGB COLOR_RGB::Blue  = ColorRGB(0    , 0     , 255   );
const ColorRGB COLOR_RGB::Black = ColorRGB(0    , 0     , 0     );
const ColorRGB COLOR_RGB::White = ColorRGB(255  , 255   , 255   );
/* clang-format on */
}  // namespace step