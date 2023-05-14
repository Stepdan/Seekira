#include "ff_decoder_video.hpp"

#include <core/log/log.hpp>

#include <video/ffmpeg/utils/ff_utils.hpp>

namespace step::video::ff {

FFDecoderVideo::FFDecoderVideo(AVStream* stream)
{
    STEP_LOG(L_INFO, "FFDecoder creating");

    if (!open_stream(stream))
        STEP_THROW_RUNTIME("Can't create FFDecoderVideo: invalid stream opening!");

    STEP_LOG(L_INFO, "Try to create sample scaler for converting to BGR24");
    m_sws = SwsContextSafe(m_decoder_ctx->width, m_decoder_ctx->height, m_decoder_ctx->pix_fmt, m_decoder_ctx->width,
                           m_decoder_ctx->height, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
}

FFDecoderVideo::~FFDecoderVideo() { m_decoder_ctx.close(); }

StreamId FFDecoderVideo::get_stream_id() const { return m_stream_index; }

bool FFDecoderVideo::is_opened() const { return m_decoder_ctx.get() && avcodec_is_open(m_decoder_ctx.get()) > 0; }

FramePtr FFDecoderVideo::decode_packet(const std::shared_ptr<IDataPacket>& packet)
{
    if (!m_decoder_ctx || !packet || !packet->get_packet())
    {
        STEP_LOG(L_ERROR, "Can't decode packet: invalid codec_ctx or packet!");
        return nullptr;
    }

    int ret = m_decoder_ctx.send_packet(packet->get_packet());
    if (ret < 0)
    {
        STEP_LOG(L_ERROR, "Failed to decode packet (avcodec_send_packet): err {}", av_make_error(ret));
        return nullptr;
    }

    // Get all the available frames from the decoder
    FramePtr frame_ptr{nullptr};
    while (ret >= 0)
    {
        ret = m_decoder_ctx.recieve_frame(m_frame.get());
        if (ret < 0)
        {
            // Those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret != AVERROR_EOF && ret != AVERROR(EAGAIN))
                STEP_LOG(L_TRACE, "Failed to decode packet (avcodec_receive_frame): err {}", av_make_error(ret));
            else
                STEP_LOG(L_WARN, "Failed to decode packet (avcodec_receive_frame): err {}", av_make_error(ret));

            //av_packet_free(&packet);
            return nullptr;
        }

        if (!m_sws)
            STEP_THROW_RUNTIME("Can't decode frame: no sws!");

        try
        {
            const auto channels_count = utils::get_channels_count(PixFmt::BGR);
            FramePtr frame_ptr = std::make_shared<Frame>();
            frame_ptr->stride = m_decoder_ctx->width * channels_count;
            frame_ptr->pix_fmt = PixFmt::BGR;
            frame_ptr->size = {static_cast<size_t>(m_decoder_ctx->width), static_cast<size_t>(m_decoder_ctx->height)};
            {
                Frame::DataTypePtr dest[3] = {frame_ptr->data(), nullptr, nullptr};
                int dest_linesize[3] = {static_cast<int>(frame_ptr->stride), 0, 0};
                sws_scale(m_sws.get(), m_frame->data, m_frame->linesize, 0, m_frame->height, dest, dest_linesize);
            }

            // Check is ff start_time is microseconds?
            frame_ptr->ts = Microseconds(frame_start_time(m_frame.get()));
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Exception handled during FramePtr creation with sws");
            //av_packet_free(&packet);
            return nullptr;
        }
    }

    //av_packet_free(&packet);
    return frame_ptr;
}

bool FFDecoderVideo::open_stream(AVStream* stream)
{
    try
    {
        if (!stream)
        {
            STEP_LOG(L_ERROR, "Can't create FFDecoderVideo: empty stream!");
            return false;
        }

        int ret;
        m_codec = CodecSafe(stream->codecpar->codec_id);
        m_decoder_ctx = DecoderContextSafe(m_codec.get());

        ret = avcodec_parameters_to_context(m_decoder_ctx.get(), stream->codecpar);
        if (ret < 0)
        {
            STEP_LOG(L_ERROR, "Unable fill codec context");
            return false;
        }

        m_decoder_ctx.open(m_codec.get());

        m_stream_index = stream->index;
        m_start_pts = (stream->start_time != AV_NOPTS_VALUE) ? stream->start_time : 0;
        m_time_base = av_q2d(stream->time_base);
        m_pts_clock = m_start_pts;
        STEP_LOG(L_INFO, "Codec opened with params: stream index {}, start pts {}, time base {}", m_stream_index,
                 m_start_pts, m_time_base);

        return true;
    }
    catch (...)
    {
        STEP_LOG(L_ERROR, "Exception handled during FFDecoderVideo creation");
        return false;
    }
}

double FFDecoderVideo::frame_pts(AVFrame* frame)
{
    if (!m_decoder_ctx)
    {
        STEP_LOG(L_WARN, "Can't provide frame_pts: empty codec ctx!");
        return 0.0;
    }

    double pts = frame->pkt_dts;
    double time_base = m_time_base;

    if (pts == AV_NOPTS_VALUE)
        pts = frame->pts;

    if (pts == AV_NOPTS_VALUE)
        pts = m_pts_clock + time_base;

    pts += frame->repeat_pict * (time_base * 0.5);
    m_pts_clock = pts;

    return pts;
}

TimestampFF FFDecoderVideo::frame_start_time(AVFrame* frame)
{
    if (!m_decoder_ctx)
    {
        STEP_LOG(L_WARN, "Can't provide frame_start_time: empty codec ctx!");
        return 0.0;
    }

    double pts = frame_pts(frame) - m_start_pts;
    return m_start_time + pts * m_time_base;
}

double FFDecoderVideo::clock() const
{
    double pts = m_pts_clock - m_start_pts;
    return m_start_time + pts * time_base();
}

}  // namespace step::video::ff