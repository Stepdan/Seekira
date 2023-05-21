#pragma once

#include "data_packet.hpp"

#include <video/frame/interfaces/frame.hpp>

namespace step::video::ff {

class IStream
{
public:
    virtual ~IStream() = default;

    virtual TimeFF get_duration() const = 0;
    virtual TimestampFF get_position() = 0;
    virtual DataPacketPtr read() = 0;
    virtual void request_seek(TimestampFF time, const std::shared_ptr<IStream>& result_checker) = 0;
    virtual void do_seek() = 0;
    virtual bool get_seek_result() = 0;

    virtual void terminate() = 0;
    virtual bool is_terminated() const = 0;

    virtual void release_internal_data() = 0;

    //---
    virtual MediaType get_media_type() const = 0;

    virtual FramePtr read_frame() = 0;  // *STEP
};

using StreamPtr = std::shared_ptr<IStream>;

}  // namespace step::video::ff