#pragma once

#include <video/ffmpeg/interfaces/types.hpp>
#include <video/ffmpeg/interfaces/types_safe.hpp>

namespace step::video::ff {

class ReaderEvent
{
public:
    enum class Type
    {
        Undefined,
        Play,
        Pause,
        Stop,
        StepForward,
        StepBackward,
        RewindForward,
        RewindBackward,
        SetPosition,
    };

public:
    explicit ReaderEvent(Type t, TimestampFF ts = AV_NOPTS_VALUE) : m_type(t), m_timestamp(ts) {}

    Type get_type() const noexcept { return m_type; }
    TimestampFF get_timestamp() const noexcept { return m_timestamp; }

private:
    Type m_type{Type::Undefined};
    TimestampFF m_timestamp{AV_NOPTS_VALUE};
};

}  // namespace step::video::ff