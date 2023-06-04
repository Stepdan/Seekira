#pragma once

#include "types.hpp"

#include <string>

namespace step::video::ff {

class IReader
{
public:
    ~IReader() = default;

    virtual bool open_file(const std::string& filename) = 0;
    virtual void start() = 0;
    virtual TimeFF get_duration() const = 0;
    virtual TimestampFF get_position() const = 0;

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