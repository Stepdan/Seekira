#pragma once

#include "stream.hpp"

namespace step::video::ff {

class IStreamReader
{
protected:
    virtual ~IStreamReader() = default;

public:
    virtual TimeFF get_duration() const = 0;
    virtual StreamPtr get_stream(StreamId stream_id) = 0;
    virtual void release_stream(StreamId stream_id) = 0;
    virtual int get_stream_count() const = 0;
    virtual int get_active_stream_count() const = 0;
};

}  // namespace step::video::ff