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

namespace {
const int MAX_FPS = 240;
// Max bitrate according to ISO/IEC 11172 (mpeg-1 video sequence header) indicates VBR:
// the rate is in units of 400 bits/s, maximum number of units 0x3FFFF.
const int64_t MPEG1_MAX_BITRATE = 0x3FFFF * 400ll;
}  // namespace

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

bool compare_tag(uint32_t lTag, const std::string& rTag)
{
    const char* const lTagStr = reinterpret_cast<const char*>(&lTag);
    const size_t len = std::min(sizeof(lTag), rTag.size());
    return strncmp(lTagStr, rTag.c_str(), len) == 0;
}

const AVRational& get_available_fps(const AVStream* stream)
{
    /// Для некоторых контейнеров (например wmv) avg_frame_rate часто невалидный,
    /// в таком случае мы возвращаем r_frame_rate.
    /// avg_frame_rate берётся из контейнера (если есть)
    /// r_frame_rate вычисляется из time base и минимальной длительности пакета,
    /// что может давать большую погрешность, особенно для черезстрочной развёртки.
    if (stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0)
        return stream->avg_frame_rate;

    return stream->r_frame_rate;
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

void ParserFF::reopen()
{
    auto input = m_input;
    AVDictionary* options = nullptr;

    if (!m_filename.empty())
        m_input = std::make_shared<AVFormatInputFF>(m_filename);
    else
        STEP_THROW_RUNTIME("Empty filename in reopen ParserFF");

    {
        int err = 0;

        do
        {
            err = avformat_find_stream_info(m_input->context(), NULL);
        } while (err == AVERROR(EAGAIN));

        if (err < 0)
            STEP_THROW_RUNTIME("Can't find stream info {} in reopen ParserFF", m_filename);
    }

    if (input->context()->duration != AV_NOPTS_VALUE)
        m_input->context()->duration = input->context()->duration;

    // Sometimes a new stream can be found while reading the file.
    // In this case the number of streams detected via find_stream_info will differ from the number of streams in the old input.
    m_stream_count = std::min(m_input->context()->nb_streams, input->context()->nb_streams);
    for (StreamId i = 0; i < m_stream_count; ++i)
    {
        if (input->context()->streams[i]->duration != AV_NOPTS_VALUE)
            m_input->context()->streams[i]->duration = input->context()->streams[i]->duration;

        if ((input->context()->streams[i]->avg_frame_rate.num > 0) &&
            (input->context()->streams[i]->avg_frame_rate.den > 0))
            m_input->context()->streams[i]->avg_frame_rate = input->context()->streams[i]->avg_frame_rate;
    }

    if (!is_compressed_swf())
        avformat_flush(m_input->context());
}

void ParserFF::close()
{
    // перед закрытиеи демуксера надо закрыть все кодеки,
    // иначе при закрытии кодеков память будет уже освобождена
    // надо будет сделать какое-то запоминание ссылок на созданные декодеры,
    // чтобы контролировать этот процесс (если это нужно)
    m_input.reset();
}

void ParserFF::seek(StreamId index, TimestampFF time)
{
    if (is_cover(index))
        return;

    m_current_chapter.clear();

    if (m_avi_with_repeating_pts || m_format_DAV)
    {
        // Для файлов с повторяющимися метками рандомный поиск не возможен.
        // Так же на текущий момент мы не можем определить направление поиска,
        // т.к. в cur_dts тоже написана чушь. Мы можем начать высчитывать позицию
        // стримов сами, но пока таких файлов не много, поэтому обойдемся чтением сначала.

        time = 0;
        m_prev_pkt_times.clear();
    }

    // m_prevPktTimes.count(index) indicates that we have asf file with stream where all pts = NOPTS_VALUE
    // After seek in nonzero position ffmpeg returns packets from zero position so we set time to 0
    // m_firstStreamPktReceived allow us to skip packets with pts = NOPTS_VALUE in the middle of stream @see RestorePacketPTS for details
    if (m_format_ASF && m_prev_pkt_times.count(index) > 0)
    {
        time = 0;
        m_first_stream_pkt_received[index] = false;
        m_prev_pkt_times.erase(m_prev_pkt_times.find(index));
    }

    STEP_LOG(L_DEBUG, "Seek [ {} ] to {}", index, time);
    if (time < 0)
        STEP_THROW_RUNTIME("Seek error: invalid seek time position. File {}, error stream id {}", m_filename, index);

    if (index >= m_stream_count)
        STEP_THROW_RUNTIME("Seek error: invalid seek stream index. File {}, error stream id {}", m_filename, index);

    if (m_format_IMG && time != 0)  // Для картинок можно позиционироваться только в начало
        STEP_THROW_RUNTIME("Seek error: seeking over image sources is not implemented. File {}, error stream id {}",
                           m_filename, index);

    // В FFMpeg после 2.7.1 не работает seek по timestamp для изображений из-за изменившегося поведения ff_img_read_packet. AVIOContext в ней модифицируется, изменяются pos/read_bytes.
    if (m_format_IMG && time == 0)
        av_seek_frame(m_input->context(), index, 0, AVSEEK_FLAG_BYTE);

    // определяем, как нужно позиционироваться: или по файлу, или по временным меткам
    // У MPEG-PS есть баг -- при позиционировании в 0 одного потока, метка второго сожет сбиваться,
    // поэтому для такого позиционирования переключаемся в бинарный режим.
    bool seek_binary =
        is_binary_seek() || (step::utils::str_contains(m_input->context()->iformat->long_name, "MPEG-PS") && time == 0);

    if (m_format_MpegTs)
    {
        seek_binary = true;
        if (index != get_seek_stream())
            STEP_THROW_RUNTIME("Seek @ mpegts: skipping seek with provided index. File {}, error stream id {}",
                               m_filename, index);
    }

    for (StreamId i = 0; i < m_stream_count; i++)
    {
        const AVStream* const stream = m_input->context()->streams[i];
        const AVRational* const time_base = &stream->time_base;
        m_gen_pts[i] = av_rescale(time, time_base->den, AV_SECOND * (int64_t)time_base->num);
    }

    /// берем временную метку, на которую нужно сделать Seek
    TimestampFF stream_time = m_gen_pts[index];

    /// грубо определяем направление
    /// @warning cur_dts не является частью публичного API и его поведение может измениться
    /// @note мы испольуем dts, который начинается с 0, а не с pStream->start_time,
    /// поэтому смещать pts нужно после определения направления seek-а!
    const AVStream* const stream = m_input->context()->streams[index];
    //bool seek_forward = ((stream->cur_dts != AV_NOPTS_VALUE) && (stream->cur_dts < stream_time));

    // Попробуем определить напралвение через последний запомненный dts для стрима
    if (!m_last_seek_ts.contains(index))
    {
        STEP_LOG(L_ERROR, "Can't find last seek ts for stream {}", index);
        return;
    }
    bool seek_forward = ((m_last_seek_ts[index] != AV_NOPTS_VALUE) && (m_last_seek_ts[index] < stream_time));

    //VLOG(L2_TS) << "Seek " << (seekForward? "forward" : "backward" )
    //	<< "["<<index << "]: "<<  pStream->cur_dts << "p to " << static_cast<long long>(streamTime) << "p\n";

    /// для каждого потока временная метка может быть своей, т.к. метки внутри файла могут начинаться не с 0
    /// в этом случае делаем сдвиг по времени
    TimestampFF start_time = stream->start_time;
    if (start_time != AV_NOPTS_VALUE)
        stream_time += start_time;

    if (m_format_SWF)  ///< swf тоже не поддерживает позиционирование
    {
        if (seek_forward)
            return;  ///< ничего не делаем - это позиционирование вперед, просто дочитаем пакеты

        if (is_compressed_swf())
        {
            reopen();  ///< compressed swf не позиционируется вообще
            return;
        }
        time = 0;            ///< swf позиционируется только в начало
        seek_binary = true;  ///< и только по файлу
    }

    if ((stream->codecpar->codec_id == AV_CODEC_ID_APNG) && time == 0)
        m_first_stream_pkt_received[index] = false;

    if (seek_binary)
    {
        int64_t stream_pos = av_rescale(get_size(), time, get_duration());
        int res = av_seek_frame(m_input->context(), index, stream_pos, AVSEEK_FLAG_BYTE);

        if (res >= 0)
            return;
    }

    /// делаем позиционирование
    if (step::utils::str_contains(m_input->context()->iformat->long_name, "QuickTime"))
    {
        int res = avformat_seek_file(m_input->context(), -1, INT64_MIN, time, INT64_MAX, 0);
        if (res >= 0)
            return;
    }

    int res = seek_forward ? avformat_seek_file(m_input->context(), index, stream_time, stream_time, INT64_MAX, 0)
                           : avformat_seek_file(m_input->context(), index, INT64_MIN, stream_time, stream_time, 0);

    if (res >= 0)
        return;

    if (!time)
    {
        /// для некоторых файлах (AVI) жесткий seek в 0 не работает, делаем seek с отступом правой границы
        for (uint64_t pos = 0; pos < AV_SECOND * 5; pos += 100000)
        {
            res = avformat_seek_file(m_input->context(), -1, INT64_MIN, 0, pos, AVSEEK_FLAG_ANY);
            if (res >= 0)
                return;
        }
    }

    STEP_THROW_RUNTIME("Invalid seek!");
}

std::shared_ptr<IDataPacket> ParserFF::read()
{
    AVPacket* pkt = read_packet();
    if (!pkt)
        return;

    // Запоминаем dts, чтобы использовать в seek вместо stream->cur_dts
    m_last_seek_ts[pkt->stream_index] = pkt->dts;

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

bool ParserFF::is_compressed_swf() const
{
    if (!m_format_SWF)
        return false;

    char buf[4] = {};

    std::ifstream file;
    file.open(m_filename, std::ios::binary);

    if (!file.is_open())
        STEP_THROW_RUNTIME("Can't open file: file is absent or you haven't access rights (is_compressed_swf)");

    file.read(buf, 3);
    file.close();

    const std::string compressed_swf_header("CWS");

    return buf == compressed_swf_header;
}

bool ParserFF::is_cover(StreamId index) const
{
    if (!m_input->context()->streams || !m_input->context()->streams[index])
        return false;

    return (m_input->context()->streams[index]->disposition & AV_DISPOSITION_ATTACHED_PIC) > 0;
}

bool ParserFF::is_binary_seek() const { return m_format_DAV || m_format_PIX || m_format_VOB || m_format_VRO; }

bool ParserFF::find_stream_info()
{
    m_contains_invalid_stream_id = false;

    for (unsigned int i = 0; i < m_input->context()->nb_streams; i++)
    {
        m_last_seek_ts.clear();
        m_last_seek_ts[m_input->context()->streams[i]->index] = AV_NOPTS_VALUE;
    }

    if (m_format_name == FORMAT_FILE::FORMAT_MP4)
        detect_mp4drm();

    if (step::utils::str_contains(m_format_name, "mxf"))
    {
        /// @todo Временная заглушка - текущая сборка ffmpeg падает, когда jpeg2000 есть в mxf
        for (unsigned int i = 0; i < m_input->context()->nb_streams; i++)
        {
            if (m_input->context()->streams[i]->codecpar->codec_id == AV_CODEC_ID_JPEG2000)
            {
                STEP_LOG(L_ERROR, "Error: could not find codec parameters: jpeg2000 in mxf");
                return false;
            }
        }
    }

    {
        int err = 0;

        do
        {
            err = avformat_find_stream_info(m_input->context(), nullptr);
        } while (err == AVERROR(EAGAIN));

        if (err < 0)
        {
            STEP_LOG(L_ERROR, "Error: could not find codec parameters");
            return false;
        }
    }

    m_stream_count = m_input->context()->nb_streams;

    std::set<int> streamsId;
    std::map<AVMediaType, StreamId> first_streams_by_type;
    std::multimap<AVMediaType, StreamId> default_streams_by_type;

    for (unsigned int i = 0; i < m_stream_count; ++i)
    {
        AVStream const* const stream = m_input->context()->streams[i];
        if (!stream)
            continue;

        int const streamId = stream->id;
        streamsId.insert(streamId);
        if (streamId == -1)
            m_contains_invalid_stream_id = true;

        // There can be several default stream of each type or no default stream at all.
        // So we keep information about first stream of each type for using
        // them as default in case of no default stream was set for this particular type.
        auto* codec_params = stream->codecpar;
        if (!codec_params)
            continue;

        auto type = codec_params->codec_type;
        bool const default_flag = stream->disposition & AV_DISPOSITION_DEFAULT;

        if (first_streams_by_type.find(type) == first_streams_by_type.end())
            first_streams_by_type.insert({type, StreamId(i)});

        if (default_flag)
            default_streams_by_type.insert({type, StreamId(i)});
    }

    for (auto const& first_stream : first_streams_by_type)
        if (default_streams_by_type.find(first_stream.first) == default_streams_by_type.end())
            default_streams_by_type.insert(first_stream);

    std::vector<bool>(m_stream_count, false).swap(m_default_streams);
    for (auto const& default_stream : default_streams_by_type)
        m_default_streams[default_stream.second] = true;

    m_first_stream_pkt_received.resize(m_stream_count);

    // Если эти значения не равны, это говорит о том, что источник имеет два или больше потока с одинаковыми ID
    // Такая ситуация может возникнуть, например, на битых файлах, у которых все потоки имеют stream->id = 0
    if (streamsId.size() != m_stream_count)
    {
        m_contains_invalid_stream_id = true;
        STEP_LOG(L_INFO, "Source has one more invalid streams with wrong ID");
    }

    if (step::utils::str_contains(m_input->context()->iformat->long_name, "MPEG-TS"))
    {
        /// Неизвестно, насколько распространена такая ошибка, поэтому
        /// ограничемся пока контейнером mpegts
        m_dts_shifts.resize(m_stream_count, 0LL);
    }

    if (step::utils::str_contains(m_input->context()->iformat->long_name, "MPEG-PS"))
    {
        for (unsigned int i = 0; i < m_input->context()->nb_streams; i++)
        {
            // // иногда startTime больше first_dts.
            // if (m_input->context()->streams[i]->first_dts != AV_NOPTS_VALUE &&
            //     m_input->context()->streams[i]->first_dts < m_input->context()->streams[i]->start_time)
            // {
            //     m_input->context()->streams[i]->start_time = m_input->context()->streams[i]->first_dts;
            // }

            if (!m_format_VOB && m_input->context()->streams[i]->codecpar->codec_id == AV_CODEC_ID_DVD_NAV &&
                m_input->context()->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_DATA)
            {
                STEP_LOG(L_WARN,
                         "Invalid input file extension: MPEG-PS with DVD_NAV data stream detected in NOT .vob or .dat "
                         "file. m_formatVOB set true");
                m_format_VOB = true;
            }
        }
    }

    detect_fps();

    if (!is_compressed_swf())
        avformat_flush(m_input->context());

    const StreamId index = get_seek_stream();
    STEP_ASSERT(index < m_stream_count, "Can't provide stream info: seek stream id {} < stream count {}", index,
                m_stream_count);

    seek(index, 0);

    return true;
}

void ParserFF::detect_mp4drm()
{
    static const std::string MP4_ATOM_DRMI = "drmi";
    static const std::string MP4_ATOM_DRMS = "drms";

    for (size_t i = 0; i < m_input->context()->nb_streams; ++i)
    {
        const AVCodecParameters* const codec_par = m_input->context()->streams[i]->codecpar;
        if (codec_par->codec_id == AV_CODEC_ID_AAC &&
            (compare_tag(codec_par->codec_tag, MP4_ATOM_DRMI) || compare_tag(codec_par->codec_tag, MP4_ATOM_DRMS)))
        {
            STEP_THROW_RUNTIME("Unsupported format - DRM encrypted M4P: {}", m_filename);
        }
    }
}

void ParserFF::detect_fps()
{
    if (m_format_IMG)
    {
        /// отключаем для картинок
        return;
    }

    int video_streams = 0;
    for (unsigned int i = 0; i < m_stream_count; i++)
    {
        AVCodecParameters* pCodec = m_input->context()->streams[i]->codecpar;
        if (pCodec && pCodec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (pCodec->codec_id != AV_CODEC_ID_JPEG2000 && pCodec->codec_id != AV_CODEC_ID_GIF &&
                pCodec->codec_id != AV_CODEC_ID_PNG && pCodec->codec_id != AV_CODEC_ID_APNG &&
                pCodec->codec_id != AV_CODEC_ID_BMP && pCodec->codec_id != AV_CODEC_ID_TIFF &&
                pCodec->codec_id != AV_CODEC_ID_MJPEG)
            {
                /// исключаем данные кодеки из рассмотрения, т.к. они чаще всего используются для кодирования обложек альбомов
                /// внутри аудио файлов
                video_streams++;
            }
        }
    }

    if (!video_streams)
    {
        /// не нашли видео потоков
        return;
    }

    struct CountAndPosition
    {
        TimestampFF start_pts = AV_NOPTS_VALUE;
        int count = 0;
    };

    std::vector<CountAndPosition> packets;
    packets.resize(m_stream_count);

    std::shared_ptr<IDataPacket> p;
    int64_t current_fps = AV_NOPTS_VALUE;
    int64_t prev_fps = AV_NOPTS_VALUE;
    int prev_fps_count = 0;
    while ((p = read()))
    {
        StreamId index = p->get_stream_index();
        TimestampFF pts = p->pts();
        CountAndPosition& info = packets[index];

        if (pts == AV_NOPTS_VALUE)
        {
            info.count++;  ///< не забываем учитывать пакеты без птс во фраймрейте
            //// ждем пакет с временной меткой
            continue;
        }

        if (info.start_pts == AV_NOPTS_VALUE)
        {
            info.start_pts = pts;
            info.count = 0;  ///< сбросим счётчик, чтобы считать начиная от этой метки
        }
        info.count++;

        TimeFF diff = pts - info.start_pts;

        /// пропускаем первый пакет
        if (!diff)
            continue;

        /// выходим, если текущая метка превысила 5 сек, а видео до сих пор нет
        if (diff > 5 * AV_SECOND)
        {
            current_fps = AV_NOPTS_VALUE;
            break;
        }

        /// пропускаем все, кроме видео
        if (p->get_media_type() != MediaType::Video)
            continue;

        if (info.count < 10)
            continue;

        AVStream* stream = m_input->context()->streams[index];
        if (!stream)
            continue;

        current_fps = av_rescale((info.count - 1), AV_SECOND, diff);

        prev_fps_count = (current_fps == prev_fps) ? (prev_fps_count + 1) : 0;
        prev_fps = current_fps;

        if (current_fps < 25)
        {
            info.count = 0;
            continue;
        }

        if (diff > AV_SECOND / 2)
        {
            /// проверям корректность на первой половине секунды
            auto rat = get_available_fps(stream);
            const int den = rat.den;
            const int num = rat.num;
            int64_t res = av_rescale(100 * current_fps, den, num);
            const AVFieldOrder fieldOrder = stream->codecpar->field_order;
            if ((res > 90 && res < 110) || (res > 190 && res < 210 && fieldOrder > AV_FIELD_PROGRESSIVE))
            {
                /// если реальный отличается от значения, которое указано в контейнере,
                /// менее, чем на 10%, то считаем информацию в контейнере верной.
                /// если отличается от указанного контейнера в 2 раза (с погрешностью 10%),
                /// то считаем, что видео с черезстрочной развёрткой.
                break;
            }

            if (diff > 3 * AV_SECOND || prev_fps_count > 5)
            {
                /// выходим, если длительность видео превысила 3 сек
                /// внутри fps лежит реальное значение FPS

                stream->avg_frame_rate.num = (int)current_fps;
                stream->avg_frame_rate.den = 1;
                break;
            }
        }
    }
}

void ParserFF::detect_bitrate()
{
    // вычисление битрейта потоков, у которых демуксер не смог получить битрейт
    // надо переделать, чтобы субтитры в вычислениях не учитывались
    // а для аудио битрейт был фиксированным

    int64_t total_bitrate = 0;

    TimeFF duration = get_duration();
    if (duration > 0)
    {
        int64_t file_bitrate = 8 * av_rescale(get_size(), AV_SECOND, duration);

        file_bitrate = (std::min)(
            file_bitrate,
            int64_t(INT_MAX));  /// @todo Нормально избежать переполнения int в AVCodecContext и AVFormatContext
        file_bitrate = (std::max)(int64_t(1024), file_bitrate);
        total_bitrate = file_bitrate;
    }
    int null_bitrate_count = 0;
    int null_bitrate_count_audio = 0;
    int null_bitrate_count_video = 0;
    // вычитаем из общего битрейта битрейты потоков
    for (StreamId i = 0; i < m_stream_count; i++)
    {
        int64_t& bitrate = m_input->context()->streams[i]->codecpar->bit_rate;

        if (m_input->context()->streams[i]->codecpar->codec_id == AV_CODEC_ID_MPEG1VIDEO &&
            bitrate == MPEG1_MAX_BITRATE)
            bitrate = 0;  // rewriting original value!

        if (bitrate > 0)
        {
            total_bitrate -= bitrate;
        }
        else
        {
            // при расчете учитываем только аудио и видео потоки
            // вклад других типов потоков не существенненый
            const auto codec_type = m_input->context()->streams[i]->codecpar->codec_type;
            null_bitrate_count_audio += (int)(codec_type == AVMEDIA_TYPE_AUDIO);
            null_bitrate_count_video += (int)(codec_type == AVMEDIA_TYPE_VIDEO);
            null_bitrate_count += (int)(codec_type == AVMEDIA_TYPE_AUDIO || codec_type == AVMEDIA_TYPE_VIDEO);
        }
    }

    const int64_t DEFAULT_VIDEO_BITRATE = 4000000;
    const int64_t DEFAULT_AUDIO_BITRATE = 256000;

    if (!null_bitrate_count)
        return;

    STEP_LOG(L_DEBUG, "Some streams have wrong bitrate. Trying to recover them...");
    if (total_bitrate <= 0)
    {
        STEP_LOG(L_DEBUG, "Can't determine total bitrate! Default values are used!");
        for (StreamId i = 0; i < m_stream_count; i++)
        {
            if (m_input->context()->streams[i]->codecpar->bit_rate)
                continue;

            switch (m_input->context()->streams[i]->codecpar->codec_type)
            {
                case AVMEDIA_TYPE_VIDEO:
                    m_input->context()->streams[i]->codecpar->bit_rate = DEFAULT_VIDEO_BITRATE;
                    m_custom_bitrates[i] = DEFAULT_VIDEO_BITRATE;
                    break;
                case AVMEDIA_TYPE_AUDIO:
                    m_input->context()->streams[i]->codecpar->bit_rate = DEFAULT_AUDIO_BITRATE;
                    m_custom_bitrates[i] = DEFAULT_AUDIO_BITRATE;
                    break;
                default:
                    break;
            }
        }
        return;
    }

    int average_bitrate = (int)(total_bitrate / null_bitrate_count);  /// Возможно переполнение int

    if (!null_bitrate_count_audio || !null_bitrate_count_video || average_bitrate < DEFAULT_AUDIO_BITRATE)
    {
        // если не опреден битрейт у потоков одинакового типа - делаем его одинаковым для всех потоков
        // или если средний битрейт получился очень маленьким
        for (StreamId i = 0; i < m_stream_count; i++)
        {
            if (m_input->context()->streams[i]->codecpar->bit_rate)
                continue;

            switch (m_input->context()->streams[i]->codecpar->codec_type)
            {
                case AVMEDIA_TYPE_VIDEO:
                case AVMEDIA_TYPE_AUDIO:
                    m_input->context()->streams[i]->codecpar->bit_rate = average_bitrate;
                    m_custom_bitrates[i] = average_bitrate;
                    break;
                default:
                    break;
            }
        }
        return;
    }

    // иначе - сначала распределяем битрейт по аудио
    for (StreamId i = 0; i < m_stream_count; i++)
    {
        if (!m_input->context()->streams[i]->codecpar->bit_rate &&
            m_input->context()->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            m_input->context()->streams[i]->codecpar->bit_rate = DEFAULT_AUDIO_BITRATE;
            m_custom_bitrates[i] = DEFAULT_AUDIO_BITRATE;
            total_bitrate -= DEFAULT_AUDIO_BITRATE;
        }
    }
    // все, что осталось - по видео
    average_bitrate = (int)(total_bitrate / null_bitrate_count_video);

    for (StreamId i = 0; i < m_stream_count; i++)
    {
        if (!m_input->context()->streams[i]->codecpar->bit_rate &&
            m_input->context()->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_input->context()->streams[i]->codecpar->bit_rate = average_bitrate;
            m_custom_bitrates[i] = average_bitrate;
        }
    }
}

TimeFF ParserFF::get_duration() const { return m_input ? m_input->context()->duration : 0; }

int64_t ParserFF::get_size() const
{
    if (!m_input)
        return 0;

    int64_t total_size = avio_size(m_input->context()->pb);
    if (total_size < 0)
    {
        total_size = avio_tell(m_input->context()->pb);
        if (total_size < 0)
            total_size = 0;
    }

    if (!total_size && m_format_IMG && m_input->context()->url[0] != '\0')
    {
        /// Для image2 неопределен m_input->pb, поэтому приходится получать размер через boost::filesystem
        total_size = std::filesystem::file_size(std::filesystem::path(m_input->context()->url));
    }

    return total_size;
}

StreamId ParserFF::get_seek_stream()
{
    if (m_input->context()->nb_streams <= 0)
        return -1;

    int bestIndex = av_find_best_stream(m_input->context(), AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
    if (bestIndex < 0)
    {
        bestIndex = av_find_best_stream(m_input->context(), AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);
        if (bestIndex < 0)
        {
            bestIndex = 0;
        }
    }

    return bestIndex;
}

}  // namespace step::video::ff