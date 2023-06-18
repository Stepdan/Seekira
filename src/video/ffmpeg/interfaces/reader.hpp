#pragma once

#include "types.hpp"

#include <video/frame/interfaces/frame_interfaces.hpp>

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

class IReaderEventObserver
{
public:
    virtual ~IReaderEventObserver() = default;

    virtual void on_reader_state_changed(ReaderState) { STEP_UNDEFINED("on_reader_state_changed is undefined!"); }
};

class IReaderEventSource
{
public:
    virtual ~IReaderEventSource() = default;

    virtual void register_observer(IReaderEventObserver* observer) = 0;
    virtual void unregister_observer(IReaderEventObserver* observer) = 0;
};

class IReader : public IReaderEventSource, public IFrameSource
{
public:
    ~IReader() = default;

    virtual bool open_file(const std::string& filename) = 0;
    virtual void start() = 0;
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