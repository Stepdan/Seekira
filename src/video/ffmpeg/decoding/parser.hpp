#pragma once

#include "decoder_video.hpp"

#include <video/ffmpeg/interfaces/format_codec.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <functional>
#include <map>

namespace step::video::ff {

class AVFormatInputFF
{
public:
    AVFormatInputFF(const std::string& filename);
    ~AVFormatInputFF();

    AVFormatContext* context() const noexcept { return m_context; }

    AVFormatContext* operator->() const { return context(); }

private:
    AVFormatContext* m_context{nullptr};
};

class ParserFF
{
public:
    ParserFF();
    ~ParserFF();

    bool open_file(const std::string& filename);

    void seek(StreamId index, TimestampFF time);
    std::shared_ptr<IDataPacket> read();

    TimeFF get_duration() const;
    StreamId get_seek_stream();
    int64_t get_size() const;
    int get_stream_count() const;
    TimeFF get_stream_duration(StreamId index) const;
    MediaType get_stream_type(StreamId index) const;

    FormatCodec get_format_codec(StreamId index) const;  // *STEP

private:
    void reopen();
    void close();

    AVPacket* read_packet();
    void restore_packet_pts(AVPacket*);

    TimeFF calc_container_shift(StreamId index);

    bool is_codec_found(const AVFormatContext* ctx, AVCodecID codec_id);
    bool is_compressed_swf() const;
    bool is_cover(StreamId index) const;  // Detect if current video stream contains only cover art
    bool is_binary_seek() const;

    bool find_stream_info();
    TimeFF find_overall_stream_duration();

    void detect_mp4drm();
    void detect_fps();
    void detect_bitrate();

    bool detect_vob_chapters();
    bool detect_vob_chapters(StreamId stream_id, std::vector<int64_t>& stream_durations);

    using AVPacketUnique = std::unique_ptr<AVPacket, std::function<void(AVPacket*)>>;
    void detect_time_shift(const std::vector<AVPacketUnique>& first_packets);
    void detect_runtime_timeshift(AVPacket const* const pkt);  // Заполняет m_timeShift и m_dtsShifts при чтении

    bool detect_other_info();

    std::vector<AVPacketUnique> get_first_packets();

    int seek_to_zero();

private:
    struct ChapterVOB
    {
        TimestampFF pts_start{AV_NOPTS_VALUE};
        TimestampFF pts_end{AV_NOPTS_VALUE};
        TimeFF time_shift{0};
        int64_t pos_start{-1};
        int64_t pos_end{-1};
    };

    using ChapterList = std::vector<ChapterVOB>;

private:
    int m_stream_count{0};
    std::string m_filename;
    std::vector<TimestampFF> m_gen_pts;
    std::vector<TimeFF> m_time_shift;
    TimestampFF m_first_pkt_pos{AV_NOPTS_VALUE};
    std::string m_format_name;
    std::shared_ptr<AVFormatInputFF> m_input;
    std::vector<TimestampFF> m_dts_shifts;

    std::map<StreamId, TimestampFF> m_last_seek_ts;

    std::vector<bool> m_first_stream_pkt_received;
    std::map<StreamId, int64_t>
        m_custom_bitrates;  ///< Переопределённые битрейты потоков. Храним отдельно, т.к. в AVFormatContext они могут перезаписаться при очередном чтении хедера внутри потока.

    int m_default_chapter_list_index = -1;
    bool m_contains_invalid_stream_id = false;
    bool m_avi_with_repeating_pts = false;
    size_t m_audio_probe_count = 0;
    TimeFF m_min_stream_duration = MAX_DURATION;
    TimeFF m_max_stream_duration = MAX_DURATION;

    std::map<StreamId, int> m_current_chapter;  ///< индекс текущего чаптера в стримах
    std::map<StreamId, TimeFF> m_stream_durations;
    std::map<StreamId, ChapterList> m_chapters;  ///< таблица разделов внутри VOB или VRO для кажого стрима
    std::map<StreamId, std::pair<TimestampFF, TimeFF>> m_prev_pkt_times;  ///< <индекс потока, <pts, duration>>
    std::vector<bool> m_default_streams;                                  ///< Default flag for each stream

    bool m_format_SWF = false;
    bool m_format_IMG = false;  ///< признак того, что читаем картинку и нужно дублирование кадров
    bool m_format_VOB = false;
    bool m_format_VRO = false;
    bool m_format_PIX = false;
    bool m_format_MpegTs = false;
    bool m_format_DAV = false;
    bool m_format_ASF = false;
};

}  // namespace step::video::ff