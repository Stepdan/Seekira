#pragma once

#include "data_packet.hpp"

#include <video/ffmpeg/interfaces/format_codec.hpp>

namespace step::video::ff {

class IDemuxer
{
public:
    virtual ~IDemuxer() = default;

    virtual TimeFF get_duration() const = 0;
    virtual TimeFF get_stream_duration(StreamId stream) const = 0;

    virtual int get_stream_count() const = 0;
    virtual MediaType get_stream_type(StreamId index) const = 0;

    virtual void enable_stream(StreamId stream, bool value) = 0;

    virtual TimestampFF seek(TimestampFF time) = 0;
    virtual bool is_eof_reached() = 0;
    virtual std::shared_ptr<IDataPacket> read(StreamId stream) = 0;
    virtual void release_internal_data(StreamId stream) = 0;

    virtual FormatCodec get_format_codec(StreamId) const = 0;  // *STEP

    virtual StreamId get_best_video_stream_id() = 0;  // *STEP
};

using DemuxerPtr = std::shared_ptr<IDemuxer>;

}  // namespace step::video::ff