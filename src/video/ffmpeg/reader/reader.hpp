#pragma once

#include "reader_event.hpp"

#include <core/base/interfaces/event_handler_list.hpp>
#include <core/threading/thread_worker.hpp>
#include <core/threading/thread_utils.hpp>
#include <core/threading/thread_pool_execute_policy.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <video/ffmpeg/interfaces/reader.hpp>
#include <video/ffmpeg/decoding/stream_reader.hpp>
#include <video/ffmpeg/decoding/decoder_video.hpp>

#include <queue>

namespace step::video::ff {

class ReaderFF : public IReader, public threading::ThreadWorker
{
public:
    ReaderFF(ReaderMode mode = ReaderMode::All);
    ~ReaderFF();

public:
    bool open_file(const std::string& filename) override;
    void start() override;
    TimeFF get_duration() const override;
    TimestampFF get_position() const override;
    ReaderState get_state() const override;

    void play() override;
    void pause() override;
    void stop() override;
    void step_forward() override;
    void step_backward() override;
    void rewind_forward() override;
    void rewind_backward() override;
    void set_position(TimestampFF) override;

public:
    void register_observer(IFrameSourceObserver* observer) override;
    void unregister_observer(IFrameSourceObserver* observer) override;

    void register_observer(IReaderEventObserver* observer) override;
    void unregister_observer(IReaderEventObserver* observer) override;

protected:
    virtual void reader_process_frame(video::FramePtr frame) {}

private:
    void play_impl();
    void pause_impl();
    void stop_impl();
    void step_forward_impl();
    void step_backward_impl();
    void rewind_forward_impl();
    void rewind_backward_impl();
    void set_position_impl(TimestampFF);

private:
    //void run_worker() override;
    //void stop_worker() override;
    void worker_thread() override;
    void thread_worker_stop_impl() override;

    void set_reader_state(ReaderState);
    void seek(TimestampFF pos);
    void read_frame();
    bool need_break_reading(bool verbose = false) const;
    bool need_handle_frame();

private:
    void add_reader_event(ReaderEvent);
    void handle_event(ReaderEvent);
    bool has_event() const;

private:
    std::shared_ptr<ParserFF> m_parser{nullptr};
    std::shared_ptr<IDemuxer> m_demuxer{nullptr};
    std::shared_ptr<IStreamReader> m_stream_reader{nullptr};
    StreamPtr m_stream{nullptr};

    std::string m_filename;

    mutable std::mutex m_event_guard;
    std::queue<ReaderEvent> m_event_queue;

    // Не придумал ничего лучше для позиционирования на один кадр назад
    // запоминаем длину последнего кадра и делаем seek(position - last_frame_position)
    FramePtr m_last_valid_frame;
    TimeFF m_prev_duration{0};
    size_t m_invalid_counter{0};

    step::EventHandlerList<IFrameSourceObserver, threading::ThreadPoolExecutePolicy<0>> m_frame_observers;
    step::EventHandlerList<IReaderEventObserver, threading::ThreadPoolExecutePolicy<0>> m_reader_observers;

    ReaderMode m_mode{ReaderMode::Undefined};
    ReaderState m_state{ReaderState::Undefined};

    mutable std::mutex m_read_guard;
    std::condition_variable m_read_cnd;
    std::atomic_bool m_continue_reading{false};

    // Need handle frame
    bool m_skip_frame_handle{false};  // Насильно отключает handle кадров
    bool m_need_handle_after_force_set_pos{
        false};  // После любого специального set_position/read_frame надо обработать кадр
    bool m_is_last_key_frame{false};  // Является ли след. кадр в стриме ключевым
};

}  // namespace step::video::ff