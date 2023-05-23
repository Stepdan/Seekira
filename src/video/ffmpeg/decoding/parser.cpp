#include "parser.hpp"
#include "data_packet.hpp"

#include <core/log/log.hpp>

#include <core/base/types/media_types.hpp>
#include <core/base/utils/string_utils.hpp>

#include <video/ffmpeg/utils/utils.hpp>
#include <video/ffmpeg/utils/media_types_utils.hpp>

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

std::string make_extension(const std::string& str)
{
    // Предполагаем, что пришло что-то типа ".MOV"
    // Убираем точку и приводим к нижнему регистру

    STEP_ASSERT(!str.empty(), "Invalid str for make_extension!");
    auto ext = str.substr(1, str.length());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

struct PacketDesc
{
    TimestampFF m_pts;
    TimestampFF m_pos;
    TimeFF m_duration;
    size_t m_size;
};

bool get_current_packet_position(AVFormatContext* ctx, StreamId stream, PacketDesc& packet)
{
    while (true)
    {
        AVPacket pkt;
        av_init_packet(&pkt);
        int res = read_frame_fixed(ctx, &pkt);
        if (res < 0)
        {
            return false;
        }
        if (pkt.pts != AV_NOPTS_VALUE && static_cast<StreamId>(pkt.stream_index) == stream)
        {
            packet = {pkt.pts, pkt.pos, pkt.duration, size_t(pkt.size)};
            av_packet_unref(&pkt);
            return true;
        }
        av_packet_unref(&pkt);
    }
}

bool get_time_at_position(AVFormatContext* ctx, TimestampFF seek_pos, TimestampFF& packet_ts, TimestampFF& packet_pos,
                          StreamId stream)
{
    if (av_seek_frame(ctx, -1, seek_pos, AVSEEK_FLAG_BYTE) < 0)
    {
        assert(0);
        return false;
    }

    PacketDesc pkt = {};
    bool res = get_current_packet_position(ctx, stream, pkt);

    packet_ts = pkt.m_pts;
    if (res && pkt.m_pos < 0)
        packet_pos = seek_pos;
    else
        packet_pos = pkt.m_pos;

    return res;
}

bool get_min_time_at_position(AVFormatContext* ctx, int64_t seek_pos, TimestampFF& packet_ts, int64_t& packet_pos,
                              StreamId stream)
{
    const size_t PROBE_SIZE = 10;

    if (av_seek_frame(ctx, -1, seek_pos, AVSEEK_FLAG_BYTE) < 0)
    {
        assert(0);
        return false;
    }

    bool res = false;
    packet_ts = AV_NOPTS_VALUE;
    packet_pos = -1;
    for (size_t i = 0; i < PROBE_SIZE; ++i)
    {
        PacketDesc pkt = {};

        const bool curRes = get_current_packet_position(ctx, stream, pkt);
        res = res || curRes;
        if (curRes == false)
            break;

        if (packet_ts == AV_NOPTS_VALUE)
            packet_ts = pkt.m_pts;
        else if (pkt.m_pts != AV_NOPTS_VALUE)
            packet_ts = std::min(packet_ts, pkt.m_pts);

        if (packet_pos < 0)
        {
            if (pkt.m_pos < 0)
                packet_pos = seek_pos;
            else
                packet_pos = pkt.m_pos;
        }
    }

    if (res == true)
    {
        // seek: set the position like after call of GetTimeAtPosition
        TimestampFF pts;
        TimestampFF pos;
        get_time_at_position(ctx, seek_pos, pts, pos, stream);
    }

    return res;
}

bool find_end_of_chapter(AVFormatContext* ctx,    ///< контекст
                         TimestampFF search_pos,  ///< начальная позиция для поиска
                         int64_t max_size,  ///< максимальный объем данных для чтения
                         TimestampFF& end_pts,  ///< последняя временная метка закончившегося чаптера
                         TimestampFF& new_pts,  ///< начальная временная метка нового чаптера
                         TimestampFF& new_pos,     ///< позиция начала нового чаптера
                         int64_t& data_precessed,  ///< кол-во прочитанных байтов
                         StreamId stream)  ///< индекс потока, для которого производится поиск
{
    PacketDesc prev_pkt = {};

    if (search_pos <= 0)
    {
        if (!get_time_at_position(ctx, -search_pos, prev_pkt.m_pts, prev_pkt.m_pos, stream))
            return false;
    }
    else
    {
        prev_pkt.m_pos = search_pos;
        prev_pkt.m_pts = end_pts;
    }

    const int64_t start_pos = prev_pkt.m_pos;
    PacketDesc pkt = prev_pkt;

    while (get_current_packet_position(ctx, stream, pkt))
    {
        if (pkt.m_pos < 0)
            pkt.m_pos = prev_pkt.m_pos + prev_pkt.m_size;

        if (pkt.m_pos < prev_pkt.m_pos)
        {
            /// игнорируем такие пакеты, т.к. они из прошлого
            assert(0);
            continue;
        }

        new_pts = pkt.m_pts;
        new_pos = pkt.m_pos;
        data_precessed = pkt.m_pos - start_pos;
        if (llabs(pkt.m_pts - prev_pkt.m_pts) > AV_SECOND)
        {
            const size_t MAX_PTS_REORDER_PROBE_SIZE = 10;

            end_pts = prev_pkt.m_pts +
                      prev_pkt.m_duration * 2;  // Add one extra duration to solve possible audio stream intersection.
            for (size_t i = 0; i < MAX_PTS_REORDER_PROBE_SIZE; ++i)
            {
                const bool res = get_current_packet_position(ctx, stream, pkt);
                if (res == false)
                    break;
                if (pkt.m_pts < new_pts)
                    new_pts = pkt.m_pts;
            }
            return true;
        }

        if ((max_size > 0) && (data_precessed > max_size))
        {
            /// не смогли найти конец чаптера на указанном отрезке файла
            return false;
        }

        const TimestampFF max_stream_pts = std::max(prev_pkt.m_pts, pkt.m_pts);
        prev_pkt = pkt;
        prev_pkt.m_pts = max_stream_pts;
    }
    end_pts = prev_pkt.m_pts +
              prev_pkt.m_duration * 2;  // Add one extra duration to solve possible audio stream intersection.
    if (pkt.m_pos >= 0)
        new_pos = pkt.m_pos;
    else if (prev_pkt.m_pos >= 0)
        new_pos = prev_pkt.m_pos + prev_pkt.m_size;
    new_pts = AV_NOPTS_VALUE;  /// конец файла
    return true;
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

ParserFF::ParserFF() {}

ParserFF::~ParserFF() { close(); }

bool ParserFF::open_file(const std::string& filename)
{
    std::filesystem::path path(filename);
    if (filename.empty() || !std::filesystem::is_regular_file(path))
    {
        STEP_LOG(L_ERROR, "Can't open file in ff parser: invalid filename {}!", filename);
        return false;
    }

    auto extension = make_extension(path.extension().string());
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

    if (!find_stream_info())
    {
        m_input.reset();
        STEP_THROW_RUNTIME("Can't find stream info during open file {}. Format name {}, streams count {}", m_filename,
                           m_format_name, m_stream_count);
    }

    return true;
}

int ParserFF::get_stream_count() const { return m_input ? m_stream_count : 0; }

TimeFF ParserFF::get_stream_duration(StreamId index) const { return m_input ? m_input->context()->duration : 0; }

MediaType ParserFF::get_stream_type(StreamId index) const
{
    if (!m_input || m_stream_count <= index)
        return MediaType::Undefined;

    switch (m_input->context()->streams[index]->codecpar->codec_type)
    {
        case AVMEDIA_TYPE_AUDIO:
            return MediaType::Audio;
        case AVMEDIA_TYPE_VIDEO:
            return MediaType::Video;
        default:
            return MediaType::Undefined;
    }
}

FormatCodec ParserFF::get_format_codec(StreamId index) const
{
    if (!m_input || index >= m_stream_count)
        STEP_THROW_RUNTIME("Can't provide stream codec info for stream {}", index);

    FormatCodec info;
    info.codec_par = m_input->context()->streams[index]->codecpar;
    info.fps = get_available_fps(m_input->context()->streams[index]);
    info.image_flag = m_format_IMG;

    return info;
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

    // m_prev_pktTimes.count(index) indicates that we have asf file with stream where all pts = NOPTS_VALUE
    // After seek in nonzero position ffmpeg returns packets from zero position so we set time to 0
    // m_first_stream_pkt_received allow us to skip packets with pts = NOPTS_VALUE in the middle of stream @see RestorePacketPTS for details
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
    /// @note мы испольуем dts, который начинается с 0, а не с stream->start_time,
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
    //	<< "["<<index << "]: "<<  stream->cur_dts << "p to " << static_cast<long long>(streamTime) << "p\n";

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
        return nullptr;

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

AVPacket* ParserFF::read_packet()
{
    AVPacket* pkt;
    int res;

    while (true)
    {
        pkt = create_packet(0);
        res = read_frame_fixed(m_input->context(), pkt);
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
                 m_input->context()->nb_streams);
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
    // so for Seek with time = 0 perform m_first_stream_pkt_received cleaning
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
    m_time_shift.resize(m_stream_count);
    m_gen_pts.resize(m_stream_count);

    std::fill(std::begin(m_gen_pts), std::end(m_gen_pts), 0);
    std::fill(std::begin(m_time_shift), std::end(m_time_shift), 0);

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
                         "Invalid input file extension: MPEG-PS with DVD_NAV data stream detected in "
                         "NOT .vob or .dat "
                         "file. m_formatVOB set true");
                m_format_VOB = true;
            }
        }
    }

    /*
	 * @warning
	 * For CODEC_ID_APNG, you cannot perform positioning until you manually determine the duration.
	 * After each positioning at 0, packets begin to arrive with different timestamps.
	 * This will lead to an incorrect duration of stream.
	 * So we first determine the duration for APNG.
	 * ffmpeg bug, fixed from version 4.1
	 */
    if (step::utils::str_contains(m_input->context()->iformat->name, "apng"))
    {
        if (m_format_VRO || m_format_VOB)
            detect_vob_chapters();
        else
            detect_other_info();

        std::vector<AVPacketUnique> const first_packets = get_first_packets();
        detect_time_shift(first_packets);
        //GetFirstPacketExtradata(first_packets);
    }
    else
    {
        std::vector<AVPacketUnique> const first_packets = get_first_packets();
        detect_time_shift(first_packets);
        //GetFirstPacketExtradata(first_packets);

        if (m_format_VRO || m_format_VOB)
            detect_vob_chapters();
        else
            detect_other_info();
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

struct SplitPoint
{
    int64_t pos;
    TimestampFF pts;

    SplitPoint()
    {
        pos = -1;
        pts = AV_NOPTS_VALUE;
    }
};
using SplitList = std::vector<SplitPoint>;

bool ParserFF::detect_vob_chapters()
{
    static StreamId InvalidIndex = -1;

    bool ret = true;
    std::vector<int64_t> streams_durations;
    StreamId first_video_index = InvalidIndex;
    StreamId first_audio_index = InvalidIndex;
    size_t videoStreamsCount = 0;
    for (StreamId i = 0; i < m_stream_count; ++i)
    {
        const auto type = m_input->context()->streams[i]->codecpar->codec_type;

        if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO)
            continue;

        if (type == AVMEDIA_TYPE_VIDEO)
            ++videoStreamsCount;

        if (type == AVMEDIA_TYPE_VIDEO && first_video_index == InvalidIndex)
            first_video_index = i;
        if (type == AVMEDIA_TYPE_AUDIO && first_audio_index == InvalidIndex)
            first_audio_index = i;

        ret = ret && detect_vob_chapters(i, streams_durations);
    }

    if (first_video_index != InvalidIndex)
        m_default_chapter_list_index = first_video_index;
    else if (first_audio_index != InvalidIndex)
        m_default_chapter_list_index = first_audio_index;
    else
        m_default_chapter_list_index = 0;

    if (ret && !streams_durations.empty())
    {
        const float MAX_RELATIVE_CHAPTER_SIZE_DIFF = 5;  // %

        bool sync_possible = videoStreamsCount == 1;
        size_t videoStreamChCount = m_chapters[first_video_index].size();
        if (sync_possible)
            for (const auto& stream : m_chapters)
                if (stream.second.size() != videoStreamChCount)
                {
                    sync_possible = false;
                    break;
                }

        m_input->context()->duration = *std::max_element(streams_durations.cbegin(), streams_durations.cend());

        if (sync_possible)
        {
            // ищем совпадающие главы и устанавливаем для них одинаковое смещение
            for (const auto& ch : m_chapters[first_video_index])
            {
                const TimestampFF mid_pos = (ch.pos_end + ch.pos_start) / 2;
                const float len = static_cast<float>(std::abs(ch.pts_end - ch.pts_start));

                for (auto& stream : m_chapters)
                {
                    if (stream.first == first_video_index)
                        continue;
                    for (auto& procCh : stream.second)
                    {
                        const float procLen = static_cast<float>(std::abs(procCh.pts_end - procCh.pts_start));
                        const float relDiff = static_cast<float>(std::abs(len / procLen - 1.) * 100.);

                        if (procCh.pos_start <= mid_pos && mid_pos <= procCh.pos_end &&
                            relDiff <
                                MAX_RELATIVE_CHAPTER_SIZE_DIFF)  // главы пересекаются и их длины отличаются не более чем на 5%
                        {
                            procCh.time_shift = ch.time_shift;
                            break;
                        }
                    }
                }
            }
        }
    }

    return ret;
}

bool ParserFF::detect_vob_chapters(StreamId stream_id, std::vector<int64_t>& streams_durations)
{
    SplitList points;
    const int64_t MB = 1000 * 1000;
    ///< Примерный размер чаптера (в байтах), которые записывают камеры или dvd-рекодеры
    const int64_t AVG_CHAPTER_SIZE = 20 * MB;
    const int64_t file_size = get_size();  ///< Размер файла

    TimestampFF new_pts = AV_NOPTS_VALUE;  ///< начальная временная метка нового чаптера
    TimestampFF new_pos = 0;               ///< позиция начала нового чаптера

    if (!get_min_time_at_position(m_input->context(), new_pos, new_pts, new_pos, stream_id))
    {
        return false;
    }

    SplitPoint p;
    p.pts = new_pts;
    p.pos = new_pos;
    points.push_back(p);

    //	bool slowSearchDetection = true;
    int64_t data_processed = 0;  ///< кол-во прочитанных байтов

    while (true)
    {
        TimestampFF end_pts = new_pts;  ///< последняя временная метка закончившегося чаптера
        TimestampFF start_pos = new_pos;
        int64_t seek_bytes = 0;  ///< сколько байт надо пропустить от текущей позиции
                                 /*
		if ((file_size - start_pos > AVG_CHAPTER_SIZE) &&
			(!slowSearchDetection))
		{
			/// небольшая эвристика для ускорения - пропускаем 2/3 предыдущего чаптера
			seek_bytes = ((std::min)(data_processed, AVG_CHAPTER_SIZE) * 2) / 3;
		}
*/
        TimestampFF search_pos =
            (seek_bytes == 0)
                ? start_pos
                : (-start_pos - seek_bytes);  ///< инвертируем, чтобы выполнить позиционирование внутри FindEndOfChapter

        int64_t search_distance = (std::max)(AVG_CHAPTER_SIZE, data_processed * 3 / 2);  ///< область поиска

        bool res = find_end_of_chapter(m_input->context(), search_pos, search_distance, end_pts, new_pts, new_pos,
                                       data_processed, stream_id);
        data_processed += seek_bytes;  ///< добавляем байты, которые пропустили

        if (!res)
        {
            /// на указанном интервале не обнаружен конец чаптера

            /// грубо ищем интервал, на котором находится конец чаптера

            int64_t diff_pts = new_pts - end_pts;
            int64_t diff_pos = new_pos - start_pos;

            int64_t test_step = AVG_CHAPTER_SIZE / 2;  ///< шаг, с которым мы проверяем PTS
            int max_count = (int)((file_size - new_pos) / test_step);
            int64_t threshold = 5;

            int i;
            for (i = 1; i < max_count; i++)
            {
                TimestampFF test_pos = new_pos + test_step * i;
                int64_t testPTS;
                int64_t testPOS;
                if (!get_time_at_position(m_input->context(), test_pos, testPTS, testPOS, stream_id))
                {
                    assert(0);
                    return false;
                }

                int64_t diff_pts2 = testPTS - end_pts;
                int64_t diff_pos2 = testPOS - start_pos;

                int64_t k1 = av_rescale(1000, diff_pts, diff_pts2);
                int64_t k2 = av_rescale(1000, diff_pos, diff_pos2);

                // Отношение изменения байтовой позиции к временной метке. Если на рассматриваемом интервале
                // нет скачка pts, то позиция и время изменяются пропорционально и их отношение ~1.
                int64_t k = (100 * k1 / k2);
                k = llabs(k - 100);

                if (k <= threshold)
                {
                    /// продолжаем поиск
                    threshold = (std::max)(k, (int64_t)2);
                    diff_pts = diff_pts2;
                    diff_pos = diff_pos2;
                    continue;
                }
                /// нашли конец чаптера
                break;
            }
            search_pos = new_pos + test_step * (i - 1);
            search_distance = AVG_CHAPTER_SIZE;

            // ищем конец на найденном прблизительно интервале
            res = find_end_of_chapter(m_input->context(), -search_pos, search_distance, end_pts, new_pts, new_pos,
                                      data_processed, stream_id);
            if (!res)
            {
                // не получилось, ищем конец без ограничения до конца файла
                res = find_end_of_chapter(m_input->context(), -search_pos, 0, end_pts, new_pts, new_pos, data_processed,
                                          stream_id);
                if (!res)
                {
                    assert(0);  /// какая-то странная ошибка
                    new_pts = AV_NOPTS_VALUE;
                    new_pos = -1;
                }
            }
        }

        p.pts = end_pts;
        p.pos = new_pos - 1;
        points.push_back(p);

        if (new_pts != AV_NOPTS_VALUE)
        {
            p.pts = new_pts;
            p.pos = new_pos;
            points.push_back(p);
            continue;
        }
        break;
    }

    av_seek_frame(m_input->context(), -1, 0, AVSEEK_FLAG_BYTE);

    ChapterList& currStreamChapters = m_chapters[stream_id];

    TimeFF clock = 0;
    points[0].pos = 0;
    points[points.size() - 1].pos = file_size;

    if (points.size() > 1)
    {
        for (unsigned int i = 0; i < points.size(); i += 2)
        {
            ChapterVOB ch;
            ch.pos_start = points[i].pos;
            ch.pos_end = points[i + 1].pos;
            ch.pts_start = points[i].pts;
            ch.pts_end = points[i + 1].pts;
            ch.time_shift = clock - ch.pts_start;
            const int64_t chapter_length = ch.pts_end - ch.pts_start;
            clock += chapter_length;
            STEP_LOG(L_TRACE, "Stream {}: {}: size {} ---> {}, start {} ({}), end: {} ({}), shift {}", stream_id, i / 2,
                     ch.pos_end - ch.pos_start, chapter_length, ch.pts_start, ch.pos_start, ch.pts_end, ch.pos_end,
                     ch.time_shift);
            currStreamChapters.push_back(ch);
        }
    }

    TimeFF duration = av_rescale(clock, AV_SECOND, 90000);  // временная шкала в dvd всегда 1/90000
    streams_durations.push_back(duration);

    return true;
}

#define MAX_FILE_SIZE_FOR_READ                                                                                         \
    30000000  ///< максимальный размер файла, когда мы определяем длительность с помощью чтения

bool ParserFF::detect_other_info()
{
    // const auto file_size = get_size();
    // const int64_t overall_duration = find_overall_stream_duration();
    // bool duration_detected = (overall_duration < MAX_DURATION);  // удалось обнаружить длительность из потоков;
    // bool redetect_duration =
    //     (m_input->context()->duration > MAX_DURATION ||
    //      m_input->context()->duration <= 0);  // признак, что надо заново определить длительность с помощью чтения

    // if (step::utils::str_contains(m_input->context()->iformat->name, "flv") &&  ///< Только для больших файлов FLV,
    //     !duration_detected &&  ///< в которых нет данных о потоках в отдельности,
    //     !redetect_duration &&  ///< но есть общая длительность файла
    //     file_size > MAX_FILE_SIZE_FOR_READ)  ///< и при этом он большой для ручного поиска
    // {
    //     return true;
    // }

    // if (step::utils::str_contains(m_input->context()->iformat->name, "aac") ||  // для аас всегда определяем длительность
    //     step::utils::str_contains(m_input->context()->iformat->name, "adts"))  // вручную
    // {
    //     redetect_duration = true;
    //     duration_detected = false;
    // }

    // if (m_input->context()->nb_streams == 1 && step::utils::str_contains(m_input->context()->iformat->name, "wav") &&
    //     m_input->context()->streams[0]->codecpar->codec_id == AV_CODEC_ID_MP3)  // вручную для MP3 внутри WAV
    // {
    //     redetect_duration = true;
    //     duration_detected = false;
    // }

    // if (m_format_SWF)  /// для swf всегда определяем длительность вручную
    // {
    //     redetect_duration = true;
    //     duration_detected = false;
    // }

    // /// проверяем, что размер файла соответствует битрейту
    // int64_t bitrate = 0;
    // for (unsigned int i = 0; i < m_input->context()->nb_streams; i++)
    // {
    //     const AVStream* const stream = m_input->context()->streams[i];
    //     if (!stream)
    //         continue;
    //     const AVCodecParameters* const pCodec = stream->codecpar;
    //     if (!pCodec)
    //         continue;

    //     int64_t codecBitrate = pCodec->bit_rate;
    //     if (!codecBitrate)
    //     {
    //         if (pCodec->codec_type == AVMEDIA_TYPE_AUDIO)
    //         {
    //             /// если битрейт для аудио не задан, то считаем, что он 128 кбит
    //             codecBitrate = 128000;
    //         }
    //         else if (pCodec->codec_type == AVMEDIA_TYPE_VIDEO && pCodec->width > 0 && pCodec->height > 0)
    //         {
    //             /// если битрейт для видео не задан, то считаем, что примерно прикидываем, исходя из размера кадра
    //             double fps = stream->avg_frame_rate.den
    //                              ? (double)stream->avg_frame_rate.num / stream->avg_frame_rate.den
    //                              : 25.0;
    //             codecBitrate =
    //                 GetDefaultVideoBitrate(CodecIDToTextID(pCodec->codec_id), pCodec->width, pCodec->height, fps);
    //         }
    //     }
    //     else
    //     {
    //         if (pCodec->codec_id == AV_CODEC_ID_MPEG1VIDEO && codecBitrate == MPEG1_MAX_BITRATE)
    //         {
    //             redetect_duration = true;
    //             duration_detected = false;
    //             break;
    //         }
    //     }
    //     bitrate += codecBitrate;
    // }

    // if (file_size > 0)
    // {
    //     if (bitrate < m_input->bit_rate)
    //     {
    //         bitrate = m_input->bit_rate;
    //     }
    //     avTime newDuration = (bitrate != 0) ? av_rescale(8 * AV_SECOND, file_size, bitrate) : 0;
    //     if (bitrate == 0)
    //     {
    //         redetect_duration = true;
    //     }

    //     avTime diff = newDuration - m_input->context()->duration;

    //     if (stristr(m_input->context()->url, ".vob") && step::utils::str_contains(m_input->context()->iformat->long_name, "MPEG-PS"))
    //     {
    //         /// для vob-файлов вычисляем длительность по битрейту
    //         /// в общем случае это не правильно, но для vob-файлов,
    //         /// в которые содержат только 1 чаптер, это будет работать
    //         m_input->context()->duration = newDuration;
    //         return true;
    //     }

    //     if (llabs(diff) > 5 * AV_SECOND)
    //     {
    //         if (!duration_detected && !redetect_duration && step::utils::str_contains(m_input->context()->iformat->name, "flv"))
    //         {
    //             redetect_duration = true;  ///< Вручную ищем длительность для битых flv
    //         }

    //         if (file_size <= MAX_FILE_SIZE_FOR_READ &&
    //             (step::utils::str_contains(m_input->context()->iformat->name, "flv") || step::utils::str_contains(m_input->context()->iformat->name, "asf") ||
    //              step::utils::str_contains(m_input->context()->iformat->name, "wma") || step::utils::str_contains(m_input->context()->iformat->name, "wmv")))
    //         {
    //             duration_detected = false;
    //             redetect_duration = true;  ///< Вручную ищем длительность для битых других, у которых размер < 30 мб
    //         }
    //     }
    // }

    // if (overall_duration < AV_SECOND && !step::utils::str_contains(m_input->context()->iformat->name, "image2"))
    // {
    //     duration_detected = false;
    //     redetect_duration = true;  ///< Вручную ищем длительность для подозрительно коротких треков, кроме картинок
    // }

    // if (redetect_duration)
    // {
    //     if (duration_detected)
    //     {
    //         m_input->context()->duration = overall_duration;
    //     }
    //     else
    //     {
    //         DetectDurationManually();
    //     }
    // }

    return true;
}

/// @brief Метод ищет минимальную длительность из всех потоков, среди видео потоков учитывается только основной.
/// Если нет ни одного медиапотока, метод вернёт MAX_DURATION, что в дальнейшем приведет к последовательному чтению
/// пакетов от начала до конца файла, чтобы гарантированно найти все потоки.
TimeFF ParserFF::find_overall_stream_duration()
{
    /// In .dav files PTS and DTS equal NOPTS value
    /// so duration may not be detected in usual way
    if (m_format_DAV && (av_seek_frame(m_input->context(), -1, 0, AVSEEK_FLAG_BYTE) >= 0))
    {
        AVPacket pkt1 = {}, *pkt = &pkt1;
        TimeFF sum_packets_duration = 0;
        while (av_read_frame(m_input->context(), pkt) >= 0)
        {
            sum_packets_duration +=
                pkt->duration != AV_NOPTS_VALUE
                    ? stream_to_global(pkt->duration, m_input->context()->streams[pkt->stream_index]->time_base)
                    : 0;
            av_packet_unref(pkt);
        }
        return sum_packets_duration;
    }

    int best_stream_index = av_find_best_stream(m_input->context(), AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
    std::vector<int64_t> durations;
    for (unsigned int i = 0; i < m_input->context()->nb_streams; i++)
    {
        AVStream* stream = m_input->context()->streams[i];

        auto codec_type = stream->codecpar->codec_type;

        /// Среди видео потоков учитывается только основной
        if (best_stream_index >= 0 && i != static_cast<unsigned>(best_stream_index) && codec_type == AVMEDIA_TYPE_VIDEO)
        {
            continue;
        }

        // Все остальные потоки, кроме видео, аудио и субтитров, не учитываем
        if (codec_type != AVMEDIA_TYPE_VIDEO && codec_type != AVMEDIA_TYPE_AUDIO)
            continue;

        if (stream->duration == AV_NOPTS_VALUE)
            continue;

        TimeFF i_duration = stream->duration;
        // пересчет в микросекунды
        i_duration = av_rescale_q(i_duration, stream->time_base, TIME_BASE_Q);
        if (i_duration > 0)
        {
            durations.push_back(i_duration);
            m_stream_durations[i] = i_duration;
        }
    }

    if (!durations.empty())
    {
        m_min_stream_duration = *std::min_element(durations.cbegin(), durations.cend());
        m_max_stream_duration = *std::max_element(durations.cbegin(), durations.cend());

        return m_max_stream_duration;
        // if (m_settings.GetDurationPolicy() == SettingsParser::DurationPolicy::Minimal)
        //     return m_min_stream_duration;
        // else if (m_settings.GetDurationPolicy() == SettingsParser::DurationPolicy::Maximal)
        //     return m_max_stream_duration;
        // else
        //     assert(!"invalid logic.");
    }

    return MAX_DURATION;
}

void ParserFF::detect_time_shift(const std::vector<AVPacketUnique>& first_packets)
{
    // Shift all streams in the container
    for (StreamId i = 0; i < m_stream_count; ++i)
    {
        TimeFF stream_shift = calc_container_shift(i);

        if (stream_shift != 0)
        {
            STEP_LOG(L_WARN, "All packets in stream {} will be shifted by {}", i, stream_shift);
            m_time_shift[i] = stream_shift;
        }
    }

    // Shift streams separately
    for (const auto& pkt : first_packets)
    {
        if (pkt == nullptr)
            continue;

        int64_t const pts = pkt->pts;
        int64_t const dts = pkt->dts;

        if (pts == AV_NOPTS_VALUE || dts == AV_NOPTS_VALUE)
            continue;

        int const idx = pkt->stream_index;

        // Packet PTS/DTS can be greater than stream duration and so will be skipped by decoder.
        if (step::utils::str_contains(m_input->context()->iformat->long_name, "MPEG-TS"))
        {
            int64_t const startTime = m_input->context()->streams[idx]->start_time;
            int64_t const duration = m_input->context()->streams[idx]->duration;
            if ((startTime != AV_NOPTS_VALUE) && (duration != AV_NOPTS_VALUE))
            {
                if ((duration + startTime) < pts)
                {
                    m_time_shift[idx] += (pts - startTime);
                    STEP_LOG(L_WARN, "First packet pts {} is greater than stream startTime + duration {}", pts,
                             duration + startTime);
                    STEP_LOG(L_WARN, "All packets in stream {} will be shifted by {}", idx, pts - startTime);
                }
            }
        }

        // \tmp\!Samples\Sorted\Video\m2ts\AVC\MPEG Audio\2012010914332_27.m2ts
        // Первый видео пакет в этом файле: pts: 0, dts: 7200.
        // Такое смещение dts идёт по всему файлу.
        if (!m_dts_shifts.empty() && (pts < dts))
            m_dts_shifts[idx] = (dts - pts);
    }
}

std::vector<ParserFF::AVPacketUnique> ParserFF::get_first_packets()
{
    constexpr size_t MAX_PROCESSING_PKT_COUNT = 50;  // Do not try forever!

    auto streams_read_complete = [this]() {
        return std::all_of(m_first_stream_pkt_received.cbegin(), m_first_stream_pkt_received.cend(),
                           [](const auto& i) { return i; });
    };

    if (seek_to_zero() < 0)
        STEP_LOG(L_WARN, "Can't seek to 0 while getting first packets!");

    std::vector<AVPacketUnique> first_packets;
    size_t processed_pkt_count = 0;
    while (!streams_read_complete() && processed_pkt_count < MAX_PROCESSING_PKT_COUNT)
    {
        AVPacketUnique pkt(create_packet(0), [](AVPacket* ptr) {
            if (!ptr)
                return;
            release_packet(&ptr);
        });

        int const res = read_frame_fixed(m_input->context(), pkt.get());

        if (res < 0)
            break;

        ++processed_pkt_count;

        int const idx = pkt->stream_index;
        if (m_first_stream_pkt_received[idx])
            continue;
        m_first_stream_pkt_received[idx] = true;

        first_packets.push_back(std::move(pkt));
    }

    if (!streams_read_complete())
        STEP_LOG(L_WARN, "Can't get first packets from all streams!");

    return std::move(first_packets);
}

int ParserFF::seek_to_zero()
{
    int res = 0;
    if ((m_format_IMG && is_binary_seek()) || (m_format_SWF && !is_compressed_swf()))
    {
        res = av_seek_frame(m_input->context(), -1, 0, AVSEEK_FLAG_BYTE);
    }
    else if (is_compressed_swf())
    {
        seek(0, 0);
        res = 0;
    }
    else
    {
        res = avformat_seek_file(m_input->context(), -1, 0, 0, 0, 0);
    }
    return res;
}

}  // namespace step::video::ff