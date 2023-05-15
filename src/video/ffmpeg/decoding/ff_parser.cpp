#include "ff_parser.hpp"
#include "ff_data_packet.hpp"

#include <core/log/log.hpp>

#include <core/base/types/media_types.hpp>
#include <core/base/utils/string_utils.hpp>

#include <video/ffmpeg/utils/ff_utils.hpp>
#include <video/ffmpeg/utils/ff_media_types_utils.hpp>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavutil/time.h>
}

#include <stdio.h>
#include <filesystem>

using namespace std::literals;

namespace step::video::ff {

// MPEG-TS timestamp fields (PTS, DTS) effectively has 33 bits that can result in timestamps discontinuity.
// If stream->pts_wrap_behavior and stream->pts_wrap_reference are correct, ffmpeg will wrap timestamps inside av_read_frame.
// If they are incorrect, we wrap timestamps ourselves.
TimestampFF mpeg_ts_time_stamp_wrap_around(TimestampFF start_time, TimestampFF current_time)
{
    TimestampFF adjust = 0;
    if (start_time > 0x0FFFFFFFF && current_time < 0x0FFFFFFFF)
        adjust = 0x1FFFFFFFF;
    return current_time + adjust;
}

}  // namespace step::video::ff

namespace step::video::ff {

AVFormatInputFF::AVFormatInputFF(const std::string& filename) : m_context(nullptr)
{
    m_context = avformat_alloc_context();
    int err_code = avformat_open_input(&m_context, filename.c_str(), nullptr, nullptr);
    if (err_code < 0)
        STEP_THROW_RUNTIME("Cant't open file {}, err: {}", filename, av_make_error(err_code));
}

AVFormatInputFF::~AVFormatInputFF()
{
    if (m_context)
        avformat_close_input(&m_context);
}
}  // namespace step::video::ff

namespace step::video::ff {

ParserFF::ParserFF() : m_video_decoder(nullptr) {}

ParserFF::~ParserFF() { stop_worker(); }

void ParserFF::load(const std::string& url_str)
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

bool ParserFF::open_file(const std::string& filename)
{
    std::filesystem::path path(filename);
    if (filename.empty() || !std::filesystem::is_regular_file(path))
    {
        STEP_LOG(L_ERROR, "Can't open file in ff parser: invalid filename {}!", filename);
        return false;
    }

    auto extension = path.extension().string();
    auto allowed_formats = get_supported_video_formats();
    if (std::ranges::none_of(allowed_formats, [&extension](const std::string& item) { return extension == item; }))
    {
        STEP_LOG(L_ERROR, "Can't open file {}: extension {} don't supported!", filename, extension);
        return false;
    }

    try
    {
        m_input = std::make_shared<AVFormatInputFF>(filename);
    }
    catch (...)
    {
        STEP_LOG(L_ERROR, "Exception handled during AVFormatInputFF creation!");
        return false;
    }

    if (!m_input->context()->iformat || !m_input->context()->iformat->name)
    {
        STEP_LOG(L_ERROR, "Unsupported format {}", extension);
        return false;
    }

    m_filename = filename;

    m_format_SWF = false;
    m_first_pkt_pos = -1;
    m_stream_count = 0;
    std::ranges::fill(m_gen_pts, 0);

    AVDictionary* options = nullptr;

    m_format_name = m_input->context()->iformat->name;
    std::string long_name = m_input->context()->iformat->long_name;
    std::string url = m_input->context()->url;

    /* clang-format off */
    m_format_IMG    = false;//Conf::IsImageContainer(m_input->iformat->name);
    m_format_SWF    = step::utils::str_contains(m_format_name, "swf");
    m_format_VOB    = step::utils::str_contains(m_format_name, "mpeg") && step::utils::str_contains(url, ".vob") || step::utils::str_contains(url, ".dat");
    m_format_VRO    = step::utils::str_contains(m_format_name, "mpeg") && step::utils::str_contains(url, ".vro");
    m_format_PIX    = step::utils::str_contains(m_format_name, "alias_pix");
    m_format_MpegTs = step::utils::str_contains(m_format_name, "mpegts");
    m_format_DAV    = step::utils::str_contains(long_name, "raw H.264 video") && step::utils::str_contains(url, ".dav");
    m_format_ASF    = step::utils::str_contains(m_format_name, "asf");
    /* clang-format on */

    m_current_chapter.clear();

    if (step::utils::str_contains(m_format_name, "mpegvideo"))
    {
        m_format_name = FORMAT_FILE::FORMAT_MPEG1VIDEO;
    }
    else if (step::utils::str_contains(m_format_name, "matroska"))
    {
        if (step::utils::str_contains(url, ".webm"))
            m_format_name = FORMAT_FILE::FORMAT_WEBM;
        else
            m_format_name = FORMAT_FILE::FORMAT_MATROSKA;
    }
    else if (step::utils::str_contains(m_format_name, "mp4"))
    {
        if (step::utils::str_contains(url, ".3g2"))
        {
            m_format_name = FORMAT_FILE::FORMAT_3G2;
        }
        else if (is_codec_found(m_input->context(), AV_CODEC_ID_H264) &&
                 is_codec_found(m_input->context(), AV_CODEC_ID_AMR_NB))
        {
            m_format_name = FORMAT_FILE::FORMAT_3G2;
        }
        else if (is_codec_found(m_input->context(), AV_CODEC_ID_AMR_WB))
        {
            m_format_name = FORMAT_FILE::FORMAT_3G2;
        }
        else if (step::utils::str_contains(url, ".3gp") || is_codec_found(m_input->context(), AV_CODEC_ID_H261) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_H263) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_H263I) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_H263P) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_AMR_NB) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_AMR_WB))
        {
            m_format_name = FORMAT_FILE::FORMAT_3GP;
        }
        else if (step::utils::str_contains(url, ".mov") || is_codec_found(m_input->context(), AV_CODEC_ID_PCM_S24LE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_S16LE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_S16BE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_S32LE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_F32LE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_F64LE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_S24BE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_S32BE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_F32BE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_F64BE) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_U8) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_PCM_S8) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_ADPCM_MS) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_ADPCM_IMA_WAV) ||
                 is_codec_found(m_input->context(), AV_CODEC_ID_WMAPRO))
        {
            m_format_name = FORMAT_FILE::FORMAT_MOV;
        }
        else
        {
            m_format_name = FORMAT_FILE::FORMAT_MP4;
        }
    }
    else if (step::utils::str_contains(m_format_name, "aac"))
    {
        /// ffmpeg имеет aac демуксер и adts муксер, у нас они идентичны  - FORMAT_FILE::FORMAT_AAC
        m_format_name = FORMAT_FILE::FORMAT_AAC;
    }
    else if (step::utils::str_contains(m_format_name, "avi"))
    {
        for (unsigned int i = 0; i < m_input->context()->nb_streams; i++)
        {
            if (m_input->context()->streams[i] && m_input->context()->streams[i]->codecpar &&
                m_input->context()->streams[i]->codecpar->codec_id == AV_CODEC_ID_NONE)
            {
                AVCodecParameters* const codec_pars = m_input->context()->streams[i]->codecpar;
                const uint32_t tagHevc = 0x43564548;  // "HEVC"
                const uint32_t tagHvc1 = 0x31637668;  // "hvc1"
                if (codec_pars->codec_tag == tagHevc)
                {
                    codec_pars->codec_tag = tagHvc1;
                    codec_pars->codec_id = AV_CODEC_ID_HEVC;
                }
            }
        }
    }
    else if (step::utils::str_contains(m_format_name, "smi"))
    {
        m_format_name = FORMAT_FILE::FORMAT_SAMI;
    }

    //FindStreamInfo

    return true;
}

std::shared_ptr<IDataPacket> ParserFF::read()
{
    AVPacket* pkt = read_packet();
    if (!pkt)
        return;

    const AVStream* const stream = m_input->context()->streams[pkt->stream_index];
    // пересчет в реальное время
    TimestampFF packet_time_pts = stream_to_global(pkt->pts, stream->time_base);
    TimestampFF packet_time_dts = stream_to_global(pkt->dts, stream->time_base);
    TimestampFF packet_duration = stream_to_global(pkt->duration, stream->time_base);

    /* clang-format off */
    MediaType type = 
        (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) ?  MediaType::Audio :
        (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) ?  MediaType::Video :
                                                                MediaType::Undefined;
    /* clang-format on */

    return DataPacketFF::create(pkt, type, packet_time_pts, packet_time_dts, packet_duration);
}

bool ParserFF::find_streams()
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

void ParserFF::worker_thread()
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
                    STEP_LOG(L_INFO, "ParserFF end of file");
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
        is_err = true;
        m_exception_ptr = std::current_exception();
        STEP_LOG(L_ERROR, "Handled exception during demuxer's run: {}", e.what());
    }
    catch (...)
    {
        is_err = true;
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

AVPacket* ParserFF::read_packet()
{
    AVPacket* pkt;
    int res;

    while (true)
    {
        pkt = create_packet(0);
        res = read_frame_fixed(m_format_input_ctx.get(), pkt);
        if (res < 0)
        {
            av_packet_unref(pkt);
            return nullptr;
        }

        if (m_first_pkt_pos == AV_NOPTS_VALUE)
            m_first_pkt_pos = pkt->pos;

        if (pkt->stream_index >= 0 && pkt->stream_index < m_stream_count)
            break;

        // иногда попадаются пакеты с номером несуществующего потока - пропускаем их
        STEP_LOG(L_WARN, "Read packet with wrong stream id: {}, max stream id: {}", pkt->stream_index,
                 m_format_input_ctx->nb_streams);
        av_packet_unref(pkt);
    }

    // Копируем пакет
    AVPacket* packet = copy_packet(pkt);  // может кинуть исключение
    av_packet_unref(pkt);

    // .dav files has all pts and dts equal NOPTS so fill them manually using duration
    // some asf files has all pts equal NOPTS, using dts instead of pts is illegal
    if (m_format_DAV || m_format_ASF)
        restore_packet_pts(packet);

    detect_runtime_timeshift(packet);
    if (!m_dts_shifts.empty())
        packet->dts -= m_dts_shifts[packet->stream_index];

    StreamId stream_index = pkt->stream_index;
    AVStream* stream = m_input->context()->streams[stream_index];

    if ((m_format_VRO || m_format_VOB) &&
        stream->codecpar->codec_type != AVMEDIA_TYPE_DATA)  // data это обычно dvd_nav поток без глав
    {
        int chapter_stream_index = (m_chapters.find(packet->stream_index) != m_chapters.end())
                                       ? packet->stream_index
                                       : m_default_chapter_list_index;
        int& current_chapter = m_current_chapter[chapter_stream_index];
        const ChapterList& chapters = m_chapters[chapter_stream_index];

        auto sz = static_cast<int>(chapters.size());
        if (packet->pos > 0)
        {
            if (current_chapter < 0 || current_chapter >= sz || chapters[current_chapter].pos_start > packet->pos ||
                chapters[current_chapter].pos_end < packet->pos)
            {
                current_chapter = -1;
                for (int i = static_cast<int>(chapters.size()) - 1; i >= 0; i--)
                {
                    if (chapters[i].pos_start <= packet->pos)
                    {
                        current_chapter = i;
                        break;
                    }
                }
                STEP_LOG(L_INFO, "Begin chapter {} for stream {}", current_chapter, pkt->stream_index);
            }
        }
        else
        {
            if (current_chapter < 0 || current_chapter >= sz)
            {
                current_chapter = 0;
            }
        }

        if (current_chapter >= 0 && current_chapter < sz)
        {
            if (packet->pts != AV_NOPTS_VALUE)
            {
                packet->pts += chapters[current_chapter].time_shift;
            }
            if (packet->dts != AV_NOPTS_VALUE)
            {
                packet->dts += chapters[current_chapter].time_shift;
            }
        }
        else
        {
            STEP_LOG(L_WARN, "Invalid chapter processing");
        }
    }
    else
    {
        if (m_format_MpegTs)
        {
            packet->pts = mpeg_ts_time_stamp_wrap_around(m_time_shift[stream_index], packet->pts);
            packet->dts = mpeg_ts_time_stamp_wrap_around(m_time_shift[stream_index], packet->dts);
        }

        // сдвиг времени
        if (stream->start_time != AV_NOPTS_VALUE)
        {
            if (packet->pts != AV_NOPTS_VALUE)
            {
                packet->pts -= m_time_shift[stream_index];
            }
            if (packet->dts != AV_NOPTS_VALUE)
            {
                packet->dts -= m_time_shift[stream_index];
            }
        }
        else
        {
            // искусственное заполнение временных меток
            if (packet->pts == AV_NOPTS_VALUE && packet->dts == AV_NOPTS_VALUE &&
                m_gen_pts[stream_index] != AV_NOPTS_VALUE)
            {
                packet->pts = m_gen_pts[stream_index];
                m_gen_pts[stream_index] = AV_NOPTS_VALUE;
            }
        }
    }

    if (m_format_SWF)  // у SWF нет признака ключевого пакета - выставляем вручную
    {
        AVStream* stream = m_input->context()->streams[packet->stream_index];
        if (stream && stream->codecpar && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            packet->flags = packet->flags | AV_PKT_FLAG_KEY;
        }
    }

    return packet;
}

void ParserFF::restore_packet_pts(AVPacket* pkt)
{
    if (!pkt)
        return;

    auto iter = m_prev_pkt_times.find(pkt->stream_index);
    if (pkt->pts != AV_NOPTS_VALUE)
    {
        if (iter != m_prev_pkt_times.end())
            m_prev_pkt_times.erase(iter);
        return;
    }

    // recover pts only for streams where all pts equal NOPTS_VALUE and ignore packets with pts = NOPTS_VALUE in the middle of stream
    if (iter == m_prev_pkt_times.end() && !m_first_stream_pkt_received[pkt->stream_index])
    {
        m_prev_pkt_times[pkt->stream_index] = {0, pkt->duration};
        pkt->pts = 0;
        pkt->dts = pkt->pts;
    }
    else if (iter != m_prev_pkt_times.end())
    {
        auto& prev_pts = iter->second.first;
        auto& prev_dur = iter->second.second;

        pkt->pts = prev_pts + prev_dur;
        pkt->dts = pkt->pts;

        prev_pts = pkt->pts;
        prev_dur = pkt->duration;
    }
    else
    {
        STEP_LOG(L_WARN, "restore_packet_pts: packet with pts = AV_NOPTS_VALUE detected, but will be skipped.");
    }
}

void ParserFF::detect_runtime_timeshift(AVPacket const* const pkt)
{
    int const idx = pkt->stream_index;
    // 1. It seems that due to acTL_offset or extra_data for .apng files ffmpeg returns
    // invalid pts value of first packet - it is greater than start time  of stream
    // 2. Value of pts of first packets differs during different reads of the same file
    // To shift all packets it is necessary to update shift value (stored in m_timeShift) at every read
    // so for Seek with time = 0 perform m_firstStreamPktReceived cleaning
    if (m_first_stream_pkt_received[idx])
        return;

    if (!(m_input->context()->streams[idx]->codecpar->codec_id == AV_CODEC_ID_APNG))
        return;

    TimestampFF start_time = m_input->context()->streams[idx]->start_time;
    TimestampFF pts = pkt->pts;

    if ((start_time != AV_NOPTS_VALUE) && (pts != AV_NOPTS_VALUE) && (pts > start_time))
    {
        m_time_shift[idx] = (pts - start_time) + calc_container_shift(idx);
        STEP_LOG(L_WARN, "First packet pts {} after seek is greater than stream start_time {}", pts, start_time);
        STEP_LOG(L_WARN, "All packets in stream {}  will be shifted by {}", idx, (pts - start_time));

        m_first_stream_pkt_received[idx] = true;
    }
}

TimeFF ParserFF::calc_container_shift(StreamId index)
{
    TimestampFF start_time = m_input->context()->start_time;
    TimeFF res = 0;
    if (start_time != AV_NOPTS_VALUE && start_time != 0)
    {
        AVStream const* const stream = m_input->context()->streams[index];
        res = av_rescale(start_time, stream->time_base.den, AV_TIME_BASE * stream->time_base.num);
    }
    return res;
}

bool ParserFF::is_codec_found(const AVFormatContext* ctx, AVCodecID codec_id)
{
    for (unsigned int i = 0; i < ctx->nb_streams; i++)
    {
        if (ctx->streams[i] && ctx->streams[i]->codecpar && ctx->streams[i]->codecpar->codec_id == codec_id)
        {
            return true;
        }
    }
    return false;
}

}  // namespace step::video::ff