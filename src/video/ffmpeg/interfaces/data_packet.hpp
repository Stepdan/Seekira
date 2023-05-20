#pragma once

#include "types.hpp"

#include <core/base/interfaces/blob.hpp>

extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/avutil.h>
}

#include <memory>

namespace step::video::ff {

class IDataPacket
{
public:
    virtual ~IDataPacket() = default;

    virtual TimeFF duration() const noexcept = 0;
    virtual void set_duration(TimeFF value) = 0;

    virtual TimestampFF pts() const noexcept = 0;
    virtual void set_pts(TimestampFF ts) = 0;

    virtual TimestampFF dts() const noexcept = 0;
    virtual void set_dts(TimestampFF ts) = 0;

    virtual TimestampFF pts_or_dts() const noexcept = 0;

    virtual MediaType get_media_type() const noexcept = 0;

    virtual StreamId get_stream_index() const = 0;
    virtual void set_stream_index(StreamId id) = 0;

    virtual bool is_key_frame() const = 0;
    virtual bool is_corrupted() const = 0;
    virtual bool is_minor_field() const = 0;

    virtual int size() const = 0;

    virtual const AVPacket* get_packet() const noexcept = 0;

    virtual std::shared_ptr<IBlob> get_data() const = 0;
};

using DataPacketPtr = std::shared_ptr<IDataPacket>;

}  // namespace step::video::ff