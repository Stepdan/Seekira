#include "settings_drawer.hpp"

#include <core/base/types/config_fields.hpp>

namespace step::proc {

SettingsDrawer::SettingsDrawer()
    : m_face_color(COLOR_RGB::Blue)
    , m_valid_face_color(COLOR_RGB::Green)
    , m_invalid_face_color(COLOR_RGB::Red)
    , m_prob_face_color(COLOR_RGB::Orange)
{
}

SettingsDrawer::SettingsDrawer(const ObjectPtrJSON& cfg)
    : m_face_color(COLOR_RGB::Blue)
    , m_valid_face_color(COLOR_RGB::Green)
    , m_invalid_face_color(COLOR_RGB::Red)
    , m_prob_face_color(COLOR_RGB::Orange)
{
    deserialize(cfg);
}

void SettingsDrawer::serialize(ObjectPtrJSON& container)
{
    const auto serialize_color = [&container](const std::string& key, ColorRGB& color) {
        auto obj_json = json::make_object_json_ptr();
        color.serialize(obj_json);
        json::set(container, key, obj_json);
    };

    serialize_color(CFG_FLD::FACE_COLOR, m_face_color);
    serialize_color(CFG_FLD::VALID_FACE_COLOR, m_valid_face_color);
    serialize_color(CFG_FLD::PROB_FACE_COLOR, m_prob_face_color);
    serialize_color(CFG_FLD::INVALID_FACE_COLOR, m_invalid_face_color);
}

void SettingsDrawer::deserialize(const ObjectPtrJSON& container)
{
    const auto deserialize_color = [&container](const std::string& key, ColorRGB& color) {
        auto obj_json = json::get_object(container, key);
        if (obj_json)
            color.deserialize(obj_json);
    };

    deserialize_color(CFG_FLD::FACE_COLOR, m_face_color);
    deserialize_color(CFG_FLD::VALID_FACE_COLOR, m_valid_face_color);
    deserialize_color(CFG_FLD::PROB_FACE_COLOR, m_prob_face_color);
    deserialize_color(CFG_FLD::INVALID_FACE_COLOR, m_invalid_face_color);
}

}  // namespace step::proc