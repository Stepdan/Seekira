#include "settings_resizer.hpp"

#include <core/base/types/config_fields.hpp>
#include <core/base/utils/find_pair.hpp>
#include <core/base/utils/string_utils.hpp>

#include <string_view>
#include <utility>

namespace {
/* clang-format off */

constexpr std::pair<step::proc::SettingsResizer::SizeMode, std::string_view> g_size_modes[] = {
    { step::proc::SettingsResizer::SizeMode::Direct             , "direct"              },
    { step::proc::SettingsResizer::SizeMode::HalfDownscale      , "half_downscale"      },
    { step::proc::SettingsResizer::SizeMode::QuaterDownscale    , "quater_downscale"    },
    { step::proc::SettingsResizer::SizeMode::ScaleByMax         , "scale_by_max"        },
    { step::proc::SettingsResizer::SizeMode::ScaleByMin         , "scale_by_min"        },
};

/* clang-format on */
}  // namespace

namespace step::utils {

template <>
std::string to_string(step::proc::SettingsResizer::SizeMode type)
{
    return find_by_type(type, g_size_modes);
}

template <>
void from_string(step::proc::SettingsResizer::SizeMode& type, const std::string& str)
{
    find_by_str(str, type, g_size_modes);
}

}  // namespace step::utils

namespace step::proc {

const std::string SettingsResizer::SETTINGS_ID = "SettingsResizer";

bool SettingsResizer::operator==(const SettingsResizer& rhs) const noexcept
{
    /* clang-format off */
    return true
        && m_interpolation == rhs.m_interpolation
        && m_frame_size == rhs.m_frame_size
        && m_size_mode == rhs.m_size_mode
    ;
    /* clang-format on */
}

void SettingsResizer::deserialize(const ObjectPtrJSON& container)
{
    step::utils::from_string(m_interpolation, json::get<std::string>(container, CFG_FLD::INTERPOLATION));
    step::utils::from_string(m_size_mode, json::get<std::string>(container, CFG_FLD::RESIZER_SIZE_MODE));
    m_frame_size.deserialize(json::get_object(container, CFG_FLD::FRAME_SIZE));
}

std::shared_ptr<task::BaseSettings> create_resizer_settings(const ObjectPtrJSON& cfg)
{
    auto settings = std::make_shared<SettingsResizer>();
    settings->deserialize(cfg);

    return settings;
}

}  // namespace step::proc