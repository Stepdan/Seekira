#include "decoder_video.hpp"

#include <core/log/log.hpp>
#include <core/base/utils/scope_exit.hpp>

#include <video/ffmpeg/utils/utils.hpp>
#include <video/ffmpeg/utils/image_utils.hpp>

namespace step::video::ff {

namespace {

// TODO Choose format
AVPixelFormat get_pixel_format(AVCodecContext*, const enum AVPixelFormat*) { return AV_PIX_FMT_BGR24; }

bool get_context(AVCodecParameters* codec_par, AVRational fps, DecoderContextSafe& out_context)
{
    DecoderContextSafe context(codec_par);

    context->codec = avcodec_find_decoder(codec_par->codec_id);
    if (!context->codec)
    {
        STEP_LOG(L_WARN, "Can't find AVCodec for {}", avcodec_get_name(codec_par->codec_id));
        return false;
    }

    context->workaround_bugs = 1;
    context->lowres = 0;
    context->idct_algo = FF_IDCT_AUTO;

    //context->get_format = get_pixel_format;

    /// UTVideo, HAP decoder requires codec_tag, it was set in parser
    if (context->codec_id == AV_CODEC_ID_UTVIDEO || context->codec_id == AV_CODEC_ID_HAP)
        context->codec_tag = codec_par->codec_tag;

    if (avcodec_open2(context.get(), context->codec, nullptr))
    {
        STEP_LOG(L_WARN, "Can't open context for {}", avcodec_get_name(codec_par->codec_id));
        return false;
    }

    context->framerate = fps;
    if (!context->framerate.num || !context->framerate.den)
    {
        STEP_LOG(L_WARN, "Frame rate is not set");
        return false;
    }

    if (context->codec)
        avcodec_flush_buffers(context.get());

    out_context.swap(context);
    return true;
}

}  // namespace

DecoderVideoFF::DecoderVideoFF() = default;

DecoderVideoFF::~DecoderVideoFF()
{
    flush(0);
    av_dict_free(&m_options);
    // закрытие декодера
    if (!m_codec || !m_codec->codec)
        return;
    avcodec_close(m_codec.get());
    m_codec.reset();
}

bool DecoderVideoFF::open(const FormatCodec& init)
{
    m_codec_id = init.codec_par->codec_id;
    m_codec_tag = init.codec_par->codec_tag;
    m_fps = init.fps;

    /// Сохраняем исходные значения из codecFormat, т.к. при открытии энкодера, так и при чтении фреймов m_codec->width и m_codec->height могут изменится.
    /// Если это произойдет, то кадры будут приведены к текущим m_width и m_height.
    m_out_frame_size =
        FrameSize(static_cast<size_t>(init.codec_par->width), static_cast<size_t>(init.codec_par->height));
    m_image_flag = init.image_flag;
    m_clock = 0;
    m_clock_reset = true;

    m_can_reopen_decoder = false;

    if (!get_context(init.codec_par, m_fps, m_codec))
        return false;

    m_clock = AV_NOPTS_VALUE;

    m_use_dts = false;
    m_first_frame_after_seek = true;

    m_best_pix_fmt = PixFmt::BGR;  // TODO change for various formats
    m_open_pix_fmt = m_codec->pix_fmt;

    m_sws_context.reset();
    STEP_LOG(L_INFO, "Try to create sample scaler for converting to best pix fmt");
    m_sws_context =
        SwsContextSafe(m_codec->width, m_codec->height, m_open_pix_fmt, m_out_frame_size.width, m_out_frame_size.height,
                       pix_fmt_to_avformat(m_best_pix_fmt), SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

    return true;
}

void DecoderVideoFF::flush(TimestampFF start_time)
{
    m_use_dts = false;
    m_first_frame_after_seek = true;

    // обнуление счетчиков для исправления глючных временных меток
    m_prev_duration_clock = 0;
    m_prev_duration_frame_pkt = 0;

    m_prev_frame.reset();
    m_start_time = start_time;

    if (m_codec->codec)
        avcodec_flush_buffers(m_codec.get());

    m_clock_reset = true;
    m_clock = AV_NOPTS_VALUE;

    std::queue<FramePtr>().swap(m_queue);
}

void DecoderVideoFF::release_internal_data()
{
    m_prev_frame.reset();

    m_sws_context.reset();
    if (m_codec && m_codec->codec)
    {
        avcodec_close(m_codec.get());
        m_can_reopen_decoder = true;
    }
    m_clock_reset = true;
}

FramePtr DecoderVideoFF::decode(const std::shared_ptr<IDataPacket>& data)
{
    while (true)
    {
        auto frame_ptr = decode_internal(data);

        if (frame_ptr)
        {
            if (!m_prev_frame)
            {
                m_prev_frame.swap(frame_ptr);
                if (data)
                    return nullptr;  // запрашиваем новые пакеты, до тех пор, пока временная метка не превысит GetStartTime()
                else
                    continue;  // мы дошли до конца потока, продолжаем извлекать забуферизированные кадры
            }

            TimestampFF new_ts = frame_ptr->ts.count();
            TimestampFF prev_ts = m_prev_frame->ts.count();
            TimeFF new_duration = new_ts - prev_ts;

            if (new_duration <= 0)  // неправильная длительность, запрашиваем еще один пакет
                return nullptr;

            m_prev_frame->duration = new_duration;
            m_prev_frame.swap(frame_ptr);
            return frame_ptr;
        }
        else
        {
            // если декодер взял пакет, но не вернул результат - нужно запросить еще один пакет
            if (data)
                return nullptr;

            // уже отдали последний кадр, поток кончился
            if (!m_prev_frame)
                return nullptr;

            auto fps = get_fps();
            TimeFF avg_frame_duration = av_rescale(AV_SECOND, fps.den, fps.num);
            m_prev_frame->duration = avg_frame_duration;

            FramePtr last_frame = nullptr;
            last_frame.swap(m_prev_frame);
            return last_frame;
        }
    }

    return nullptr;
}

FramePtr DecoderVideoFF::decode_internal(const std::shared_ptr<IDataPacket>& data)
{
    if (m_can_reopen_decoder)
    {
        // CodecParametersSafe params;
        // avcodec_parameters_from_context(params.get(), m_codec.get());
        // params->format = m_open_pix_fmt;
        // if (!get_context(params.get(), m_fps, m_codec))
        //     STEP_THROW_RUNTIME("Can't reopen decoder");

        m_can_reopen_decoder = false;
    }

    AVPacket tmp_pkt;
    STEP_SCOPE_EXIT([&tmp_pkt]() { av_packet_unref(&tmp_pkt); });

    if (!data)
    {
        memset(&tmp_pkt, 0, sizeof(tmp_pkt));
        tmp_pkt.pts = AV_NOPTS_VALUE;
        tmp_pkt.dts = AV_NOPTS_VALUE;
    }
    else
    {
        av_packet_ref(&tmp_pkt, data->get_packet());
        // обновляем временные метки в пакете
        tmp_pkt.pts = data->pts();
        tmp_pkt.dts = data->dts();
        tmp_pkt.duration = data->duration();

        // начинать декодирование можно только с ключевого пакета
        // проверка этого условия и выставление флага
        // этот флаг выставляется в методе Flush после сброса декодера перед Seek
        if (data->is_key_frame() && m_clock_reset)
        {
            // флаг, который определяет, что надо использовать метки из DTS, а не из PTS
            // т.к. на некоторых форматах PTS всегда равна NOPTS_VALUE
            // это делается для получения первой временной метки во время Seek, т.к. Seek Останавливаетя по достижении заданной временной метки.
            m_use_dts = (tmp_pkt.pts == AV_NOPTS_VALUE && tmp_pkt.dts != AV_NOPTS_VALUE);
            // сбрасываем этот флаг
            m_clock_reset = false;
        }

        if (m_use_dts)
            tmp_pkt.pts = tmp_pkt.dts;
    }

    const auto res = m_codec.send_packet(&tmp_pkt);
    if (res == AVERROR(EAGAIN))
        STEP_THROW_RUNTIME("Invalid decoder logic.");
    else if (res < 0 && res != AVERROR_EOF)
        return nullptr;

    do
    {
        FrameSafe avframe;

        const int prev_width = m_codec->width;
        const int prev_height = m_codec->height;
        AVPixelFormat prev_fmt = m_codec->pix_fmt;

        const int res = m_codec.recieve_frame(avframe.get());
        if (res == AVERROR_EOF || res == AVERROR(EAGAIN))
            break;

        const bool formatIsChanged =
            prev_width != m_codec->width || prev_height != m_codec->height || prev_fmt != m_codec->pix_fmt;

        if (avframe->top_field_first)
        {
            /// принудительно выставляем этот флаг, на некоторых dvd его почему-то нет
            avframe->interlaced_frame = 1;
        }

        // проверка на правильность работы декодера
        // какой-то из кодеков выдавал неправильные данные
        if (m_codec->pix_fmt == AV_PIX_FMT_YUV420P)
        {
            if (!avframe->data[0] || !avframe->data[1] || !avframe->data[2])
                break;  // ждем следующий пакет
        }
        else
        {
            if (!avframe->data[0])
                break;  // ждем следующий пакет
        }

        avframe->width = m_codec->width;
        avframe->height = m_codec->height;
        avframe->sample_aspect_ratio.num = 1;  // TODO m_outFrameInfo.ax;
        avframe->sample_aspect_ratio.den = 1;  // TODO m_outFrameInfo.ay;

        // обновление счетчика времени декодера m_clock
        TimestampFF new_clock = avframe->best_effort_timestamp;

        // вычисляем длительность предыдущего кадра
        if (new_clock == AV_NOPTS_VALUE && m_clock == AV_NOPTS_VALUE && m_first_frame_after_seek)
        {
            m_clock = m_start_time;
            m_prev_duration_clock = 0;
        }
        else if (new_clock >= m_clock || m_first_frame_after_seek)
        {
            /// для первого пакета вычислять длительность нельзя
            m_prev_duration_clock = m_first_frame_after_seek ? 0 : new_clock - m_clock;
            m_clock = new_clock;
        }
        else
        {
            // может получиться такая ситуация, что duration, вычисленный по pts, резко скакнет по значению и вернется на исходную позицию, например:
            // 40000->200000->40000,  и frame->pts на последнем кадре будет NOPTS.
            // best_effort в таком случае возвращает такую же метку времени, как была раньше, и newClock > m_clock условие не сработает.
            if (avframe->pts == AV_NOPTS_VALUE && m_prev_duration_frame_pkt > 0)
                m_prev_duration_clock = m_prev_duration_frame_pkt;

            m_clock += m_prev_duration_clock;
        }

        m_prev_duration_frame_pkt = (avframe->pkt_duration == 0 || avframe->pkt_duration == AV_NOPTS_VALUE)
                                        ? (m_fps.num == 0 ? 0 : TimeFF(double(m_fps.den) * AV_SECOND / m_fps.num))
                                        : avframe->pkt_duration;
        m_first_frame_after_seek = false;

        // копирование данных в новую структуру, которая отдается наружу
        FramePtr frame_ptr = nullptr;

        if (!m_sws_context)
            STEP_THROW_RUNTIME("Can't decode frame: no sws!");

        try
        {
            const auto channels_count = utils::get_channels_count(m_best_pix_fmt);

            AVFrame* frame = allocate_avframe(pix_fmt_to_avformat(m_best_pix_fmt), m_codec->width, m_codec->height,
                                              m_codec->width * channels_count);

            if (!frame)
                return nullptr;

            sws_scale(m_sws_context.get(), avframe->data, avframe->linesize, 0, avframe->height, frame->data,
                      frame->linesize);

            frame_ptr = std::make_shared<Frame>(avframe_to_frame(frame));
            av_frame_free(&frame);

            frame_ptr->ts = Microseconds(m_clock);
            frame_ptr->duration = m_prev_duration_frame_pkt;
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Exception handled during FramePtr creation with sws");
            return nullptr;
        }

        m_processed_count += static_cast<bool>(frame_ptr);
        m_queue.push(frame_ptr);
    } while (true);

    return get_next_queued_frame();
}

AVRational DecoderVideoFF::get_fps() const { return m_fps; }

FramePtr DecoderVideoFF::get_next_queued_frame()
{
    if (m_queue.empty())
        return nullptr;

    auto frame = m_queue.front();
    m_queue.pop();

    return frame;
}

}  // namespace step::video::ff

namespace step::video::ff {

// FFDecoderVideo::FFDecoderVideo(AVStream* stream)
// {
//     STEP_LOG(L_INFO, "FFDecoder creating");

//     if (!open_stream(stream))
//         STEP_THROW_RUNTIME("Can't create FFDecoderVideo: invalid stream opening!");

//     STEP_LOG(L_INFO, "Try to create sample scaler for converting to BGR24");
//     m_sws = SwsContextSafe(m_decoder_ctx->width, m_decoder_ctx->height, m_decoder_ctx->pix_fmt, m_decoder_ctx->width,
//                            m_decoder_ctx->height, AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
// }

// FFDecoderVideo::~FFDecoderVideo() { m_decoder_ctx.close(); }

// StreamId FFDecoderVideo::get_stream_id() const { return m_stream_index; }

// bool FFDecoderVideo::is_opened() const { return m_decoder_ctx.get() && avcodec_is_open(m_decoder_ctx.get()) > 0; }

// FramePtr FFDecoderVideo::decode_packet(const std::shared_ptr<IDataPacket>& packet)
// {
//     if (!m_decoder_ctx || !packet || !packet->get_packet())
//     {
//         STEP_LOG(L_ERROR, "Can't decode packet: invalid codec_ctx or packet!");
//         return nullptr;
//     }

//     int ret = m_decoder_ctx.send_packet(packet->get_packet());
//     if (ret < 0)
//     {
//         STEP_LOG(L_ERROR, "Failed to decode packet (avcodec_send_packet): err {}", av_make_error(ret));
//         return nullptr;
//     }

//     // Get all the available frames from the decoder
//     FramePtr frame_ptr{nullptr};
//     while (ret >= 0)
//     {
//         ret = m_decoder_ctx.recieve_frame(m_frame.get());
//         if (ret < 0)
//         {
//             // Those two return values are special and mean there is no output
//             // frame available, but there were no errors during decoding
//             if (ret != AVERROR_EOF && ret != AVERROR(EAGAIN))
//                 STEP_LOG(L_TRACE, "Failed to decode packet (avcodec_receive_frame): err {}", av_make_error(ret));
//             else
//                 STEP_LOG(L_WARN, "Failed to decode packet (avcodec_receive_frame): err {}", av_make_error(ret));

//             //av_packet_free(&packet);
//             return nullptr;
//         }

//         if (!m_sws)
//             STEP_THROW_RUNTIME("Can't decode frame: no sws!");

//         try
//         {
//             const auto channels_count = utils::get_channels_count(PixFmt::BGR);
//             FramePtr frame_ptr = std::make_shared<Frame>();
//             frame_ptr->stride = m_decoder_ctx->width * channels_count;
//             frame_ptr->pix_fmt = PixFmt::BGR;
//             frame_ptr->size = {static_cast<size_t>(m_decoder_ctx->width), static_cast<size_t>(m_decoder_ctx->height)};
//             {
//                 Frame::DataTypePtr dest[3] = {frame_ptr->data(), nullptr, nullptr};
//                 int dest_linesize[3] = {static_cast<int>(frame_ptr->stride), 0, 0};
//                 sws_scale(m_sws.get(), m_frame->data, m_frame->linesize, 0, m_frame->height, dest, dest_linesize);
//             }

//             // Check is ff start_time is microseconds?
//             frame_ptr->ts = Microseconds(frame_start_time(m_frame.get()));
//         }
//         catch (...)
//         {
//             STEP_LOG(L_ERROR, "Exception handled during FramePtr creation with sws");
//             //av_packet_free(&packet);
//             return nullptr;
//         }
//     }

//     //av_packet_free(&packet);
//     return frame_ptr;
// }

// bool FFDecoderVideo::open_stream(AVStream* stream)
// {
//     try
//     {
//         if (!stream)
//         {
//             STEP_LOG(L_ERROR, "Can't create FFDecoderVideo: empty stream!");
//             return false;
//         }

//         int ret;
//         m_codec = CodecSafe(stream->codecpar->codec_id);
//         m_decoder_ctx = DecoderContextSafe(m_codec.get());

//         ret = avcodec_parameters_to_context(m_decoder_ctx.get(), stream->codecpar);
//         if (ret < 0)
//         {
//             STEP_LOG(L_ERROR, "Unable fill codec context");
//             return false;
//         }

//         m_decoder_ctx.open(m_codec.get());

//         m_stream_index = stream->index;
//         m_start_pts = (stream->start_time != AV_NOPTS_VALUE) ? stream->start_time : 0;
//         m_time_base = av_q2d(stream->time_base);
//         m_pts_clock = m_start_pts;
//         STEP_LOG(L_INFO, "Codec opened with params: stream index {}, start pts {}, time base {}", m_stream_index,
//                  m_start_pts, m_time_base);

//         return true;
//     }
//     catch (...)
//     {
//         STEP_LOG(L_ERROR, "Exception handled during FFDecoderVideo creation");
//         return false;
//     }
// }

// double FFDecoderVideo::frame_pts(AVFrame* frame)
// {
//     if (!m_decoder_ctx)
//     {
//         STEP_LOG(L_WARN, "Can't provide frame_pts: empty codec ctx!");
//         return 0.0;
//     }

//     double pts = frame->pkt_dts;
//     double time_base = m_time_base;

//     if (pts == AV_NOPTS_VALUE)
//         pts = frame->pts;

//     if (pts == AV_NOPTS_VALUE)
//         pts = m_pts_clock + time_base;

//     pts += frame->repeat_pict * (time_base * 0.5);
//     m_pts_clock = pts;

//     return pts;
// }

// TimestampFF FFDecoderVideo::frame_start_time(AVFrame* frame)
// {
//     if (!m_decoder_ctx)
//     {
//         STEP_LOG(L_WARN, "Can't provide frame_start_time: empty codec ctx!");
//         return 0.0;
//     }

//     double pts = frame_pts(frame) - m_start_pts;
//     return m_start_time + pts * m_time_base;
// }

// double FFDecoderVideo::clock() const
// {
//     double pts = m_pts_clock - m_start_pts;
//     return m_start_time + pts * time_base();
// }

}  // namespace step::video::ff