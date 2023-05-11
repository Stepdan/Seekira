#include "ff_stream.hpp"

#include <video/ffmpeg/utils/ff_image_utils.hpp>

#include <core/log/log.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <algorithm>

namespace {

const AVPixelFormat g_decode_pix_fmt[] = {
    AV_PIX_FMT_BGR24,
    AV_PIX_FMT_GRAY8,
};

AVPixelFormat get_desired_pix_fmts_for_decode(AVCodecContext* /*ctx*/, const enum AVPixelFormat* pixelFormat)
{
    for (const AVPixelFormat* p = pixelFormat; *p != AV_PIX_FMT_NONE; ++p)
    {
        const auto fmt = *p;
        if (std::any_of(std::cbegin(g_decode_pix_fmt), std::cend(g_decode_pix_fmt),
                        [&fmt](AVPixelFormat item) { return item == fmt; }))
            return fmt;
    }

    return AV_PIX_FMT_NONE;
};

// https://github.com/bmewj/video-app/blob/master/src/video_reader.cpp
// av_err2str returns a temporary array. This doesn't work in gcc.
// This function can be used as a replacement for av_err2str.
static const char* av_make_error(int errnum)
{
    static char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

}  // namespace

namespace step::video::ff {

struct FFStream::Impl
{
    AVFormatContext* format_ctx{nullptr};
    const AVCodec* codec{nullptr};
    AVCodecContext* codec_ctx{nullptr};
    AVCodecParameters* codec_pars{nullptr};
    AVStream* stream{nullptr};

    int stream_index;
    AVFrame* frame{nullptr};
    AVPacket* packet{nullptr};
    SwsContext* scaler_ctx{nullptr};

    FFStreamInfo info;

    std::atomic_bool need_seek;
    Timestamp seek_ts;

    std::atomic_bool is_eof;
};

FFStream::FFStream() : m_impl(std::make_unique<Impl>()) {}

FFStream::~FFStream() { close(); }

FFStreamInfo FFStream::get_stream_info() const { return m_impl->info; }

FFStreamError FFStream::open(const std::string& filename, PixFmt pix_fmt /* = PixFmt::BGR*/,
                             FrameSize frame_size /*= FrameSize()*/)
{
    STEP_LOG(L_INFO, "Try to open FFStream: {}", filename);

    int ret;

    // Open the file using libavformat
    m_impl->format_ctx = avformat_alloc_context();
    if (!m_impl->format_ctx)
    {
        STEP_LOG(L_ERROR, "Failed to create AVFormatContext: {}", filename);
        return FFStreamError::InvalidFormatContext;
    }

    ret = avformat_open_input(&(m_impl->format_ctx), filename.c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        STEP_LOG(L_ERROR, "Failed to avformat_open_input: {}, error {}", filename, ret);
        return FFStreamError::InvalidOpen;
    }

    // retrive input stream information
    ret = avformat_find_stream_info(m_impl->format_ctx, nullptr);
    if (ret < 0)
    {
        STEP_LOG(L_ERROR, "Failed to avformat_find_stream_info: {}, error {}", filename, ret);
        return FFStreamError::InvalidInfo;
    }

    // find primary video stream
    ret = av_find_best_stream(m_impl->format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &(m_impl->codec), 0);
    if (ret < 0)
    {
        STEP_LOG(L_ERROR, "Failed to av_find_best_stream: {}, error {}", filename, ret);
        return FFStreamError::InvalidBestStream;
    }
    m_impl->stream_index = ret;
    m_impl->stream = m_impl->format_ctx->streams[m_impl->stream_index];

    // open video decoder context
    m_impl->codec_ctx = avcodec_alloc_context3(m_impl->codec);
    if (!m_impl->codec_ctx)
    {
        printf("Failed to avcodec_alloc_context3: {}", filename);
        return FFStreamError::InvalidCodecContext;
    }

    m_impl->codec_ctx->get_format = get_desired_pix_fmts_for_decode;

    // avcodec_parameters_to_context ?
    // https://github.com/bmewj/video-app/blob/master/src/video_reader.cpp

    ret = avcodec_open2(m_impl->codec_ctx, m_impl->codec, nullptr);
    if (ret < 0)
    {
        STEP_LOG(L_ERROR, "Failed to avcodec_open2: {}, error {}", filename, ret);
        return FFStreamError::InvalidCodec;
    }

    m_impl->frame = av_frame_alloc();
    if (!m_impl->frame)
    {
        STEP_LOG(L_ERROR, "Failed to av_frame_alloc: {}", filename);
        return FFStreamError::InvalidFrame;
    }

    m_impl->packet = av_packet_alloc();
    if (!m_impl->packet)
    {
        STEP_LOG(L_ERROR, "Failed to av_packet_alloc: {}", filename);
        return FFStreamError::InvalidPacket;
    }

    // Taken from Movavi
    if (m_impl->codec_ctx)
        avcodec_flush_buffers(m_impl->codec_ctx);

    STEP_LOG(L_INFO, "Try to create sample scaler: {}", filename);
    m_impl->scaler_ctx = sws_getCachedContext(
        nullptr, m_impl->stream->codecpar->width, m_impl->stream->codecpar->height,
        static_cast<AVPixelFormat>(m_impl->stream->codecpar->format), static_cast<int>(frame_size.width),
        static_cast<int>(frame_size.height), pix_fmt_to_avformat(pix_fmt), SWS_BICUBIC, nullptr, nullptr, nullptr);

    if (!m_impl->scaler_ctx)
    {
        STEP_LOG(L_ERROR, "Failed to sws_getCachedContext: {}", filename);
        return FFStreamError::InvalidSwsContext;
    }

    STEP_LOG(L_INFO, "Try to create FFStreamInfo: {}", filename);
    m_impl->info = FFStreamInfo(
        filename, m_impl->format_ctx->iformat->name, m_impl->codec->name,
        pix_fmt,  //avformat_to_pix_fmt(static_cast<AVPixelFormat>(m_impl->stream->codecpar->format))
        FrameSize(m_impl->stream->codecpar->width, m_impl->stream->codecpar->height),
        av_q2d(av_guess_frame_rate(m_impl->format_ctx, m_impl->stream, nullptr)), m_impl->stream->duration);

    STEP_LOG(L_INFO, "FFStream has been opened: {}", filename);
    return FFStreamError::None;
}

FFStreamError FFStream::close()
{
    avcodec_close(m_impl->codec_ctx);
    avformat_close_input(&(m_impl->format_ctx));

    return FFStreamError::None;
}

void FFStream::worker_thread()
{
    bool is_eof = false;
    bool is_read_err = false;
    while (!m_need_stop && !is_eof)
    {
        auto frame_ptr = read_frame();
        if (frame_ptr)
        {
            m_frame_observers.perform_for_each_event_handler(
                std::bind(&IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));
        }
    }

    if ()

        if (m_need_stop)
        {
        }
}

FramePtr FFStream::read_frame()
{
    AVIOInterruptCB while (av_read_frame(m_impl->format_ctx, m_impl->packet) >= 0)
    {
        if (m_impl->packet->stream_index != m_impl->stream_index)
        {
            av_packet_unref(m_impl->packet);
            continue;
        }

        if (auto ret = avcodec_send_packet(m_impl->codec_ctx, m_impl->packet); ret < 0)
        {
            STEP_LOG(L_ERROR, "Failed to decode packet (avcodec_send_packet): {}, err {}", m_impl->info.filename(),
                     av_make_error(ret));
            return nullptr;
        }

        if (auto ret = avcodec_receive_frame(m_impl->codec_ctx, m_impl->frame); ret == AVERROR(EAGAIN) || AVERROR_EOF)
        {
            is_eof = ret == AVERROR_EOF;
            av_packet_unref(m_impl->packet);
            continue;
        }
        else if (ret < 0)
        {
            STEP_LOG(L_ERROR, "Failed to decode packet (avcodec_receive_frame): {}, err {}", m_impl->info.filename(),
                     av_make_error(ret));
            return nullptr;
        }

        av_packet_unref(m_impl->packet);
        break;
    }

    try
    {
        const auto channels_count = utils::get_channels_count(m_impl->info.pix_fmt());

        FramePtr frame_ptr = std::make_shared<Frame>();
        frame_ptr->stride = m_impl->info.frame_size().width * channels_count;
        frame_ptr->pix_fmt = m_impl->info.pix_fmt();
        frame_ptr->size = m_impl->info.frame_size();
        // TODO ts

        int stride = static_cast<int>(frame_ptr->stride);
        switch (m_impl->info.pix_fmt())
        {
            case PixFmt::GRAY: {
                Frame::DataTypePtr dest[1] = {frame_ptr->data()};
                int dest_linesize[1] = {stride};
                sws_scale(m_impl->scaler_ctx, m_impl->frame->data, m_impl->frame->linesize, 0, m_impl->frame->height,
                          dest, dest_linesize);
                break;
            }

            case PixFmt::BGR:
                [[fallthrough]];
            case PixFmt::RGB: {
                Frame::DataTypePtr dest[3] = {frame_ptr->data(), nullptr, nullptr};
                int dest_linesize[3] = {stride, 0, 0};
                sws_scale(m_impl->scaler_ctx, m_impl->frame->data, m_impl->frame->linesize, 0, m_impl->frame->height,
                          dest, dest_linesize);
                break;
            }

            case PixFmt::BGRA:
                [[fallthrough]];
            case PixFmt::RGBA: {
                Frame::DataTypePtr dest[4] = {frame_ptr->data(), nullptr, nullptr, nullptr};
                int dest_linesize[4] = {stride, 0, 0, 0};
                sws_scale(m_impl->scaler_ctx, m_impl->frame->data, m_impl->frame->linesize, 0, m_impl->frame->height,
                          dest, dest_linesize);
                break;
            }
        };

        return frame_ptr;
    }
    catch (...)
    {
        STEP_LOG(L_ERROR, "Unknown exception during avframe -> frame");
        m_exception_ptr = std::current_exception();
        return nullptr;
    }
}

void FFStream::seek_frame() {}

}  // namespace step::video::ff