#pragma once

#include "reader_event_handler.hpp"

#include <core/threading/thread_worker.hpp>
#include <core/threading/thread_utils.hpp>
#include <core/threading/thread_pool_execute_policy.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <video/ffmpeg/decoding/stream_reader.hpp>
#include <video/ffmpeg/decoding/decoder_video.hpp>

#include <shared_mutex>

namespace step::video::ff {

class ReaderFF : public step::threading::ThreadWorker, public IFrameSource, public IReaderEventSource
{
public:
    ReaderFF();
    ~ReaderFF();

    bool open_file(const std::string& filename);
    TimeFF get_duration() const;
    TimeFF get_frame_duration() const;
    TimestampFF get_position() const;

    ReaderState get_state() const;

    void start();
    void stop();

    void play();
    void pause();

    void set_position(TimestampFF);

    void request_read();
    void request_read_prev();

public:
    void register_observer(IFrameSourceObserver* observer) override;
    void unregister_observer(IFrameSourceObserver* observer) override;

    void register_observer(IReaderEventObserver* observer) override;
    void unregister_observer(IReaderEventObserver* observer) override;

private:
    void seek(TimestampFF pos);

    void worker_thread() override;
    void thread_worker_stop_impl() override;

    void read_frame();

    void set_reader_state(ReaderState);

    bool is_correct_state() const;

    bool is_paused() const;

    bool need_break_reading();

private:
    std::shared_ptr<ParserFF> m_parser{nullptr};
    std::shared_ptr<IDemuxer> m_demuxer{nullptr};
    std::shared_ptr<IStreamReader> m_stream_reader{nullptr};

    StreamPtr m_stream{nullptr};

    std::string m_filename;

    step::EventHandlerList<IFrameSourceObserver, threading::ThreadPoolExecutePolicy<0>> m_frame_observers;
    step::EventHandlerList<IReaderEventObserver, threading::ThreadPoolExecutePolicy<0>> m_reader_observers;

    TimestampFF m_read_position{AV_NOPTS_VALUE};

    ReadingMode m_read_mode{ReadingMode::Undefined};
    ReaderState m_state{ReaderState::Undefined};

    // Не придумал ничего лучше для позиционирования на один кадр назад
    // запоминаем длину последнего кадра и делаем seek(position - last_frame_position)
    FramePtr m_last_valid_frame;
    TimeFF m_prev_duration{0};
    size_t m_invalid_counter{0};

    std::shared_mutex m_data_guard;

    std::atomic_bool m_need_read{false};
    std::mutex m_read_guard;
    std::condition_variable m_read_cnd;

    std::atomic_bool m_read_stopped{false};
    std::mutex m_read_stopped_guard;
    std::condition_variable m_read_stopped_cnd;
};

}  // namespace step::video::ff