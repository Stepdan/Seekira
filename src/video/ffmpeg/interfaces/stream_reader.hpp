#pragma once

#include "stream.hpp"
#include "format_codec.hpp"

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

    virtual FormatCodec get_format_codec(StreamId stream_id) const = 0;  // *STEP
    virtual StreamPtr get_best_video_stream() = 0;                       // *STEP
    virtual bool is_eof_reached() = 0;                                   // *STEP
};

}  // namespace step::video::ff