#include "reader.hpp"

#include <core/base/types/config_fields.hpp>

namespace step::video::ff {

void IReader::Initializer::deserialize(const ObjectPtrJSON& container)
{
    IReader::Initializer init;
    step::utils::from_string<video::ff::ReaderMode>(init.mode, json::get<std::string>(container, CFG_FLD::MODE));
}

bool IReader::Initializer::is_valid() const noexcept
{
    /* clang-format off */
    return true
        && mode != ReaderMode::Undefined
    ;
    /* clang-format on */
}

bool IReader::Initializer::operator==(const IReader::Initializer& rhs) const noexcept
{
    /* clang-format off */
    return true
        && mode == rhs.mode
    ;
    /* clang-format on */
}

}  // namespace step::video::ff