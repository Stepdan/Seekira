#pragma once

#include "types.hpp"

#include <string>

namespace step::video::ff {

enum class ReaderMode
{
    Undefined,
    KeyFrame,
    All,
};

enum class ReaderState
{
    Undefined,
    Reading,
    Paused,
    Stopped,
    EndOfFile,
    Error,
};

class IReader
{
public:
    ~IReader() = default;

    virtual bool open_file(const std::string& filename) = 0;
    virtual void start(ReaderMode mode = ReaderMode::All) = 0;
    virtual TimeFF get_duration() const = 0;
    virtual TimestampFF get_position() const = 0;
    virtual ReaderState get_state() const = 0;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void step_forward() = 0;
    virtual void step_backward() = 0;
    virtual void rewind_forward() = 0;
    virtual void rewind_backward() = 0;

    virtual void set_position(TimestampFF) = 0;
};

}  // namespace step::video::ff