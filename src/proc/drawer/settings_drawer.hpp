#pragma once

#include <core/base/types/color.hpp>

namespace step::proc {

class SettingsDrawer : public ISerializable
{
public:
    SettingsDrawer();
    SettingsDrawer(const ObjectPtrJSON& cfg);

    void set_face_color(const ColorRGB& color) { m_face_color = color; };
    void set_valid_face_color(const ColorRGB& color) { m_valid_face_color = color; };
    void set_invalid_face_color(const ColorRGB& color) { m_invalid_face_color = color; };
    void set_prob_face_color(const ColorRGB& color) { m_prob_face_color = color; };

    ColorRGB get_face_color() const noexcept { return m_face_color; }
    ColorRGB get_valid_face_color() const noexcept { return m_valid_face_color; }
    ColorRGB get_prob_face_color() const noexcept { return m_prob_face_color; }
    ColorRGB get_invalid_face_color() const noexcept { return m_invalid_face_color; }

    void serialize(ObjectPtrJSON&) override;
    void deserialize(const ObjectPtrJSON&) override;

private:
    ColorRGB m_face_color;
    ColorRGB m_valid_face_color;
    ColorRGB m_prob_face_color;
    ColorRGB m_invalid_face_color;
};

}  // namespace step::proc