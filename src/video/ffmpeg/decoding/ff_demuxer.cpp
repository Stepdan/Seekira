#include "ff_demuxer.hpp"
#include "ff_data_packet.hpp"

#include <core/log/log.hpp>

#include <video/ffmpeg/utils/ff_utils.hpp>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
}

using namespace std::literals;

namespace step::video::ff {

int FFDemuxerInterruptCallback::handler(void* obj)
{
    FFDemuxerInterruptCallback* callback_obj = reinterpret_cast<FFDemuxerInterruptCallback*>(obj);
    if (!callback_obj)
        return 0;

    // TODO check when we need to interrupt
    return 0;
}

}  // namespace step::video::ff

namespace step::video::ff {

FFDemuxer::FFDemuxer() : m_video_decoder(nullptr) {}

FFDemuxer::~FFDemuxer() { stop_worker(); }

void FFDemuxer::load(const std::string& url_str)
{
    int ret;

    if (m_format_input_ctx)
    {
        STEP_LOG(L_WARN, "Demuxer already loaded!");
        return;
    }

    if (url_str.empty())
    {
        STEP_LOG(L_WARN, "Empty url for demuxer loading!");
        return;
    }

    // TODO For rstp, etc cameras - check by predicate
    if (m_is_local_file)
    {
        STEP_LOG(L_INFO, "{} is local file, calling avdevice_register_all()", url_str);
        avdevice_register_all();
    }

    // Try to open file
    // TODO AvInputFormat ?
    m_format_input_ctx = FormatContextInputSafe(url_str, nullptr, nullptr);

    ret = avformat_find_stream_info(m_format_input_ctx.get(), nullptr);
    if (ret < 0)
    {
        STEP_LOG(L_ERROR, "Can't find stream info: {}", av_make_error(ret));
        return;
    }

    // TODO Do we need dump for always?
    av_dump_format(m_format_input_ctx.get(), 0, url_str.c_str(), 0);

    if (!find_streams())
    {
        STEP_LOG(L_ERROR, "Unable to find valid streams!");
        return;
    }

    if (m_video_streams.size() > 0)
    {
        m_video_decoder = std::make_unique<FFDecoderVideo>(m_video_streams.begin()->second);
        STEP_LOG(L_INFO, "Video decoder has been created!");
    }

    if (!m_video_decoder->is_opened())
    {
        STEP_LOG(L_ERROR, "Unable to open codec!");
        return;
    }

    STEP_LOG(L_INFO, "Media loaded successfully!");
}

bool FFDemuxer::find_streams()
{
    m_video_streams.clear();

    if (!m_format_input_ctx)
    {
        STEP_LOG(L_ERROR, "Can't find streams: empty format context!");
        return false;
    }

    AVMediaType type = AVMEDIA_TYPE_UNKNOWN;
    for (size_t i = 0; i < m_format_input_ctx->nb_streams; ++i)
    {
        if (AVMEDIA_TYPE_VIDEO == m_format_input_ctx->streams[i]->codecpar->codec_type)
        {
            auto* stream_ptr = m_format_input_ctx->streams[i];
            STEP_LOG(L_INFO, "Found video stream: index {}", stream_ptr->index);
            m_video_streams.insert({stream_ptr->index, stream_ptr});
        }
    }

    if (m_video_streams.empty())
    {
        STEP_LOG(L_WARN, "No video streams found!");
        return false;
    }

    STEP_LOG(L_INFO, "Found {} video streams", m_video_streams.size());
    return true;
}

void FFDemuxer::worker_thread()
{
    int ret;
    double clock = 0.0;
    double time_base = 0.0;
    bool is_eof = false;
    bool is_err = false;

    if (m_video_decoder->is_opened())
        time_base = m_video_decoder->time_base();

    STEP_LOG(L_INFO, "Start ff demuxer run with time base {}", time_base);

    try
    {
        while (!m_need_stop && !is_eof && !is_err)
        {
            if (!m_format_input_ctx.get())
            {
                STEP_LOG(L_ERROR, "AVFormatContext m_format_input_ctx is invalid");
                break;
            }

            auto* packet = create_packet(0);
            if (!packet)
            {
                STEP_LOG(L_ERROR, "Failed to av_packet_alloc");
                break;
            }
            auto data_packet =
                FFDataPacket::create(packet, MediaType::Video, packet->pts, packet->dts, packet->duration);

            ret = av_read_frame(m_format_input_ctx.get(), packet);
            if (ret < 0)
            {
                if (ret == AVERROR_EOF)
                {
                    STEP_LOG(L_INFO, "FFDemuxer end of file");
                    is_eof = true;
                }
                else
                {
                    STEP_LOG(L_ERROR, "Unable to read frame");
                }
                av_packet_free(&packet);
                break;
            }

            if (!m_video_decoder)
            {
                STEP_LOG(L_CRITICAL, "Video decoder is empty!");
                is_err = true;
                // TODO exception?
                break;
            }

            if (m_video_streams.contains(packet->stream_index) &&
                packet->stream_index == m_video_decoder->get_stream_id())
            {
                auto frame_ptr = m_video_decoder->decode_packet(data_packet);
                clock = m_video_decoder->clock();
                //Send frame to FFWrapper
                m_frame_observers.perform_for_each_event_handler(
                    std::bind(&IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));
            }
            else
            {
                // TODO neccessary?
                std::this_thread::sleep_for(1ms);
            }

            // Primitive sync for local playback
            int count = 0;
            if (m_is_local_file)
            {
                while (clock > av_gettime())
                {
                    if (m_need_stop)
                    {
                        //av_packet_free(&packet);
                        break;
                    }
                    std::this_thread::sleep_for(Microseconds(static_cast<size_t>(time_base)));
                    ++count;
                }

                if (count > 0)
                    STEP_LOG(L_TRACE, "Demuxer playback sync: count {}, clock {}", count, clock);
            }

            //av_packet_free(&packet);
        }
    }
    catch (const std::exception& e)
    {
        m_exception_ptr = std::current_exception();
        STEP_LOG(L_ERROR, "Handled exception during demuxer's run: {}", e.what());
    }
    catch (...)
    {
        m_exception_ptr = std::current_exception();
        STEP_LOG(L_ERROR, "Handled unknown exception during demuxer's run");
    }

    if (is_eof)
    {
        STEP_LOG(L_INFO, "Stopping demuxer because of eof");
        stop_worker();
        return;
    }

    if (is_err)
    {
        STEP_LOG(L_INFO, "Stopping demuxer because of err");
        stop_worker();
        return;
    }

    if (m_need_stop)
    {
        STEP_LOG(L_INFO, "Demuxer has been stopped because of stop required!");
        return;
    }
}

}  // namespace step::video::ff