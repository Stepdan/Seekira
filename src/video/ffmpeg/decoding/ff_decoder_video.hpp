#pragma once

#include "ff_types_safe.hpp"
#include "ff_data_packet.hpp"

#include "video/frame/interfaces/frame.hpp"

#include <video/ffmpeg/utils/ff_types.hpp>

#include <atomic>

namespace step::video::ff {

class FFDecoderVideo
{
public:
    FFDecoderVideo(AVStream* stream);
    ~FFDecoderVideo();

    FramePtr decode_packet(const std::shared_ptr<IDataPacket>& packet);

    StreamId get_stream_id() const;
    bool is_opened() const;

    TimestampFF start_time() const { return m_start_time; }
    void set_start_time(TimestampFF time) { m_start_time = time; }
    double time_base() const { return m_time_base * 1000000; }
    double clock() const;

private:
    bool open_stream(AVStream* stream);

    double frame_pts(AVFrame* frame);
    TimestampFF frame_start_time(AVFrame* frame);

private:
    StreamId m_stream_index;
    CodecSafe m_codec;
    DecoderContextSafe m_decoder_ctx;
    FrameSafe m_frame;

    SwsContextSafe m_sws;

    TimestampFF m_start_time;
    TimestampFF m_start_pts;
    double m_time_base;
    std::atomic<double> m_pts_clock;  // Equivalent to the PTS of the current frame
};

}  // namespace step::video::ff