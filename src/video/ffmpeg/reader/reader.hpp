#pragma once

#include "reader_event_handler.hpp"

#include <core/threading/thread_worker.hpp>
#include <core/threading/thread_utils.hpp>
#include <core/threading/thread_pool_execute_policy.hpp>

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <video/ffmpeg/decoding/stream_reader.hpp>
#include <video/ffmpeg/decoding/decoder_video.hpp>

namespace step::video::ff {

class ReaderFF : public step::threading::ThreadWorker, public IFrameSource, public IReaderEventSource
{
public:
    ReaderFF();
    ~ReaderFF();

    void open_file(const std::string& filename);
    TimeFF get_duration() const;

    ReaderState get_state() const;

    void start(ReadingMode mode);
    void stop();

    void seek(TimestampFF pos);

    void request_read();

public:
    void register_observer(IFrameSourceObserver* observer) override;
    void unregister_observer(IFrameSourceObserver* observer) override;

    void register_observer(IReaderEventObserver* observer) override;
    void unregister_observer(IReaderEventObserver* observer) override;

private:
    void run_worker() override;
    void stop_worker() override;
    void worker_thread() override;
    void thread_worker_stop_impl() override;

    void set_reader_state(ReaderState);

private:
    std::shared_ptr<ParserFF> m_parser{nullptr};
    std::shared_ptr<IDemuxer> m_demuxer{nullptr};
    std::shared_ptr<IStreamReader> m_stream_reader{nullptr};

    StreamPtr m_stream{nullptr};

    std::string m_filename;

    step::EventHandlerList<IFrameSourceObserver, threading::ThreadPoolExecutePolicy<0>> m_frame_observers;
    step::EventHandlerList<IReaderEventObserver, threading::ThreadPoolExecutePolicy<0>> m_reader_observers;

    threading::ThreadConditionalParam m_need_read_cnd{false};

    ReadingMode m_read_mode{ReadingMode::Undefined};
    ReaderState m_state{ReaderState::Undefined};

    mutable std::mutex m_read_guard;
    std::condition_variable m_request_read_finished_cnd;
};

}  // namespace step::video::ff