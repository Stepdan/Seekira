#pragma once

#include "ff_decoder_video.hpp"

#include <core/base/interfaces/event_handler_list.hpp>
#include <core/threading/thread_worker.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <video/ffmpeg/utils/ff_types.hpp>

#include <functional>
#include <map>

namespace step::video::ff {

constexpr TimeFF MAX_DURATION = std::numeric_limits<TimeFF>::max();

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

class ParserFF : public threading::ThreadWorker, public IFrameSource
{
public:
    ParserFF();
    ~ParserFF();

    bool open_file(const std::string& filename);

    std::shared_ptr<IDataPacket> read();

private:
    bool find_streams();

    AVPacket* read_packet();
    void restore_packet_pts(AVPacket*);
    void detect_runtime_timeshift(AVPacket const* const pkt);
    TimeFF calc_container_shift(StreamId index);

    bool is_codec_found(const AVFormatContext* ctx, AVCodecID codec_id);

    // threading::ThreadWorker
private:
    void worker_thread();

    // IFrameSource
public:
    void register_observer(IFrameSourceObserver* observer) override
    {
        m_frame_observers.register_event_handler(observer);
    }

    void unregister_observer(IFrameSourceObserver* observer) override
    {
        m_frame_observers.unregister_event_handler(observer);
    }

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

    std::vector<bool> m_first_stream_pkt_received;

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

    FormatContextInputSafe m_format_input_ctx;
    std::map<StreamId, AVStream*> m_video_streams;

    std::unique_ptr<FFDecoderVideo> m_video_decoder;

    step::EventHandlerList<IFrameSourceObserver> m_frame_observers;

    bool m_is_local_file{true};
};

}  // namespace step::video::ff