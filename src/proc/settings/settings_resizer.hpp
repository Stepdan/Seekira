#pragma once

#include <core/base/types/interpolation.hpp>
#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame_size.hpp>

namespace step::proc {

class SettingsResizer : public task::BaseSettings
{
public:
    enum class SizeMode
    {
        Undefined,
        Direct,           // Уменьшаем чётко по переданныому FrameSize
        HalfDownscale,    // Уменьшаем в 2 раза
        QuaterDownscale,  // Уменьшаем в 4 раза
        ScaleByMin,  // Масштабируем по минимальному значению из пары width/height в переданном FrameSize
        ScaleByMax,  // Масштабируем по максимальному значению из пары width/height в переданном FrameSize
    };

public:
    TASK_SETTINGS(SettingsResizer)

    SettingsResizer() = default;

    bool operator==(const SettingsResizer& rhs) const noexcept;
    bool operator!=(const SettingsResizer& rhs) const noexcept { return !(*this == rhs); }

    const video::FrameSize& get_frame_size() const noexcept { return m_frame_size; }
    InterpolationType get_interpolation() const noexcept { return m_interpolation; }
    SizeMode get_size_mode() const noexcept { return m_size_mode; }

    void set_frame_size(const video::FrameSize& value) { m_frame_size = value; }
    void set_interpolation(InterpolationType type) { m_interpolation = type; }
    void set_size_mode(SizeMode mode) { m_size_mode = mode; }

public:
    InterpolationType m_interpolation{InterpolationType::Undefined};
    SizeMode m_size_mode{SizeMode::Undefined};
    video::FrameSize m_frame_size;
};

std::shared_ptr<task::BaseSettings> create_resizer_settings(const ObjectPtrJSON&);

}  // namespace step::proc