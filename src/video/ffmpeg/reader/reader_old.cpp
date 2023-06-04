// #include "reader.hpp"

// #include <core/log/log.hpp>

// #include <core/base/utils/string_utils.hpp>

// namespace step::video::ff {

// ReaderFF::ReaderFF() {}

// ReaderFF::~ReaderFF()
// {
//     STEP_LOG(L_INFO, "ReaderFF destruction, file: {}", m_filename);
//     stop();
// }

// bool ReaderFF::open_file(const std::string& filename)
// {
//     m_parser = std::make_shared<ParserFF>();
//     if (!m_parser->open_file(filename))
//     {
//         STEP_LOG(L_ERROR, "Failed to open file {}", filename);
//         return false;
//     }

//     m_demuxer = std::make_shared<DemuxerQueue>(m_parser);
//     m_stream_reader = std::make_shared<StreamReader>(m_demuxer);

//     m_stream = m_stream_reader->get_best_video_stream();

//     set_reader_state(ReaderState::Ready);

//     m_filename = filename;

//     m_invalid_counter = 0;

//     return true;
// }

// TimeFF ReaderFF::get_duration() const
// {
//     STEP_ASSERT(m_stream, "ReaderFF can't provide duration: empty stream!");
//     return m_stream->get_duration();
// }

// TimeFF ReaderFF::get_frame_duration() const
// {
//     STEP_ASSERT(m_last_valid_frame && m_last_valid_frame->duration != AV_NOPTS_VALUE,
//                 "ReaderFF can't provide frame duration: m_last_frame_duration is empty!");
//     return m_last_valid_frame->duration;
// }

// TimestampFF ReaderFF::get_position() const
// {
//     STEP_ASSERT(m_last_valid_frame, "ReaderFF can't provide position: invalid last frame!");
//     return m_last_valid_frame->ts.count();
// }

// ReaderState ReaderFF::get_state() const { return m_state; }

// void ReaderFF::start()
// {
//     if (is_running())
//     {
//         STEP_LOG(L_WARN, "Reader already started, current mode {}", step::utils::to_string(m_read_mode));
//         return;
//     }

//     m_read_mode = ReadingMode::ByRequest;
//     m_need_read.store(false);
//     set_reader_state(ReaderState::ReadingByRequest);

//     run_worker();

//     STEP_LOG(L_INFO, "Start ReaderFF with params: state {}, mode {}", step::utils::to_string(m_state),
//              step::utils::to_string(m_read_mode));
// }

// void ReaderFF::play()
// {
//     if (!m_read_stopped)
//         return;

//     m_read_mode = ReadingMode::Continuously;
//     m_read_stopped.store(false);
//     m_need_read.store(true);
//     m_read_cnd.notify_all();
// }

// void ReaderFF::pause()
// {
//     if (is_paused())
//         return;

//     STEP_LOG(L_DEBUG, "Wait for reader stop");

//     m_need_read.store(false);

//     // ждем сигнала от цикла чтения, что цикл висит на m_read_cnd.wait
//     std::unique_lock lock(m_read_stopped_guard);
//     m_read_stopped_cnd.wait(lock, [this]() { return !is_running() || !is_correct_state() || is_paused(); });
//     if (!is_running() || !is_correct_state())
//     {
//         STEP_LOG(L_INFO, "Reader stop aborted: is_running {}, state {}", is_running(), step::utils::to_string(m_state));
//         return;
//     }

//     m_read_mode = ReadingMode::ByRequest;
//     STEP_LOG(L_DEBUG, "Reader stopped");
// }

// void ReaderFF::stop()
// {
//     pause();
//     stop_worker();
// }

// void ReaderFF::set_position(TimestampFF pos)
// {
//     // Фиксим выходы за временные границы
//     if (pos < 0)
//     {
//         STEP_LOG(L_WARN, "Set position: pos < 0, fix set to 0");
//         pos = 0;
//     }

//     // Если выходим за конец - не делаем set_position
//     // Потому что последний валидный кадр не обязан быть на метке get_duration() и мы начнем читать невалидные кадры
//     if (pos >= get_duration())
//     {
//         STEP_LOG(L_WARN, "Can't set position: over the end!");
//         return;
//     }

//     STEP_LOG(L_DEBUG, "Try to set position {} to {}", get_position(), pos);

//     // Отключаем обзерверов, чтобы не летели кадры и статусы
//     m_frame_observers.disable();
//     m_reader_observers.disable();

//     // TODO Краевые условия

//     // останавливаем чтение
//     pause();

//     // Делаем seek на нужную позицию
//     seek(pos);

//     if (m_state != ReaderState::SuccessfulSeek)
//     {
//         STEP_LOG(L_ERROR, "Unsuccsessful seek to {}", pos);
//         return;
//     }

//     // После seek мы стоим на ключевом кадре, надо дочитать до нужного места, если требуется
//     // Читаем по одному кадру, пока следующий для чтения кадр не будет нужным
//     auto next_frame_ts = m_stream->get_position();
//     // duration следующего кадра: m_stream->get_pkt_duration();
//     auto after_next_frame_ts = next_frame_ts + m_stream->get_pkt_duration();

//     // Проверяем, что дошли по таймштампам до нужного кадра (следующий при чтении - нужный)
//     const auto check_ts = [&pos, &next_frame_ts, &after_next_frame_ts]() {
//         return next_frame_ts != pos && after_next_frame_ts < pos;
//     };

//     while (!need_break_reading() && check_ts())
//     {
//         read_frame();
//         next_frame_ts = m_stream->get_position();
//         after_next_frame_ts = next_frame_ts + m_stream->get_pkt_duration();
//     }

//     // Дошли до нужного места (или не дошли из-за ошибки) - включаем обзерверов
//     m_frame_observers.enable();
//     m_reader_observers.enable();

//     if (need_break_reading())
//     {
//         if (!is_correct_state())
//         {
//             STEP_LOG(L_DEBUG, "Invalid set_position to {} because of invalid state: {}", pos,
//                      step::utils::to_string(m_state));
//             // перезапишем состояние, чтобы оно улетело подписчикам
//             set_reader_state(m_state);
//         }
//         STEP_LOG(L_DEBUG, "Invalid set_position to {}", pos);
//         return;
//     }

//     // Читаем нужный нам кадр
//     STEP_LOG(L_DEBUG, "Read frame with desired position");
//     read_frame();

//     STEP_LOG(L_DEBUG, "Position has been setted to {} [{}]", m_last_valid_frame->ts.count(), pos);

//     // Если предыдущий режим был Continiously - принудительно включаем
//     if (m_read_mode == ReadingMode::Continuously)
//         play();
// }

// void ReaderFF::seek(TimestampFF pos)
// {
//     set_reader_state(ReaderState::TryToSeek);
//     m_stream->request_seek(pos, nullptr);
//     m_stream->do_seek();
//     if (!m_stream->get_last_seek_result())
//     {
//         STEP_LOG(L_ERROR, "Invalid seek to {}", pos);
//         set_reader_state(ReaderState::InvalidSeek);
//         return;
//     }

//     STEP_LOG(L_INFO, "Succesful seek to {} [{}]", m_stream->get_position(), pos);
//     set_reader_state(ReaderState::SuccessfulSeek);
// }

// void ReaderFF::request_read()
// {
//     if (!m_read_stopped)
//     {
//         STEP_LOG(L_WARN, "Can't request read: reading not stopped!");
//         return;
//     }

//     m_read_mode = ReadingMode::ByRequest;
//     read_frame();
// }

// void ReaderFF::request_read_prev()
// {
//     if (!m_read_stopped)
//     {
//         STEP_LOG(L_WARN, "Can't request read: reading not stopped!");
//         return;
//     }

//     m_read_mode = ReadingMode::ByRequest;
//     set_position(get_position() - m_prev_duration);
// }

// void ReaderFF::set_reader_state(ReaderState state)
// {
//     m_state = state;
//     m_reader_observers.perform_for_each_event_handler(
//         std::bind(&IReaderEventObserver::on_reader_state_changed, std::placeholders::_1, m_state));
// }

// void ReaderFF::thread_worker_stop_impl()
// {
//     // m_need_stop already 'true' (see ThreadWorker::stop())
//     // Делаем notify для всех кондишнов
//     m_read_cnd.notify_all();

//     m_read_stopped_cnd.notify_all();
// }

// void ReaderFF::read_frame()
// {
//     // TODO L_TRACE
//     STEP_LOG(L_DEBUG, "Try read frame");

//     auto frame_ptr = m_stream->read_frame();
//     if (frame_ptr)
//     {
//         m_invalid_counter = 0;
//         m_prev_duration = m_last_valid_frame ? m_last_valid_frame->duration : 0;
//         m_last_valid_frame = frame_ptr;

//         m_frame_observers.perform_for_each_event_handler(
//             std::bind(&IFrameSourceObserver::process_frame, std::placeholders::_1, frame_ptr));
//     }
//     else
//     {
//         ++m_invalid_counter;
//         STEP_LOG(L_WARN, "Read invalid frame, count {}", m_invalid_counter);
//     }

//     STEP_LOG(L_DEBUG, "Read {} frame with ts {}, duration {}", frame_ptr ? "valid" : "invalid",
//              m_last_valid_frame->ts.count(), m_last_valid_frame->duration);
// }

// void ReaderFF::worker_thread()
// {
//     while (!need_break_reading() && !m_stream->is_eof_reached())
//     {
//         try
//         {
//             std::unique_lock lock(m_read_guard);

//             // Перед wait помечаем, что чтение на паузе
//             m_read_stopped.store(true);
//             m_read_stopped_cnd.notify_all();

//             m_read_cnd.wait(lock, [this]() { return need_break_reading() || m_need_read; });

//             {
//                 if (need_break_reading())
//                     break;

//                 m_read_stopped.store(false);
//                 read_frame();
//             }
//         }
//         catch (const std::exception& e)
//         {
//             STEP_LOG(L_ERROR, "Failed read frame: {}", e.what());
//             m_exception_ptr = std::current_exception();
//             break;
//         }
//         catch (...)
//         {
//             STEP_LOG(L_ERROR, "Unhandled exception during read frame");
//             m_exception_ptr = std::current_exception();
//             break;
//         }
//     }

//     m_need_read.store(false);
//     m_read_stopped.store(true);
//     m_read_stopped_cnd.notify_all();

//     if (!m_need_stop && m_exception_ptr)
//         set_reader_state(ReaderState::Error);
//     else if (m_invalid_counter > 10)
//         set_reader_state(ReaderState::Error);

//     if (!m_need_stop && m_stream->is_eof_reached())
//         set_reader_state(ReaderState::EndOfFile);

//     STEP_LOG(L_INFO, "ReaderFF worker thread finished!");
// }

// bool ReaderFF::is_correct_state() const
// {
//     /* clang-format off */
//     return false
//         || m_state == ReaderState::Ready
//         || m_state == ReaderState::ReadingByRequest
//         || m_state == ReaderState::ReadingContiniously
//         || m_state == ReaderState::SuccessfulSeek
//     ;
//     /* clang-format on */
// }

// bool ReaderFF::is_paused() const { return !m_need_read && m_read_stopped; }

// bool ReaderFF::need_break_reading()
// {
//     if (!is_correct_state())
//         return true;

//     return m_need_stop || m_invalid_counter > 10;
// }

// /* clang-format off */
// void ReaderFF::register_observer(IFrameSourceObserver* observer) { m_frame_observers.register_event_handler(observer); }
// void ReaderFF::register_observer(IReaderEventObserver* observer) { m_reader_observers.register_event_handler(observer); }
// void ReaderFF::unregister_observer(IFrameSourceObserver* observer) { m_frame_observers.unregister_event_handler(observer); }
// void ReaderFF::unregister_observer(IReaderEventObserver* observer) { m_reader_observers.unregister_event_handler(observer); }
// /* clang-format on */

// }  // namespace step::video::ff