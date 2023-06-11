#include "reader.hpp"

#include <core/log/log.hpp>
#include <core/exception/assert.hpp>

#include <core/base/utils/string_utils.hpp>

namespace {

constexpr step::video::ff::TimestampFF STEP_VALUE = 2 * step::video::ff::AV_SECOND;

}  // namespace

namespace step::video::ff {

/*
    Здесь мы под
        std::unique_lock lock(m_read_guard);
        std::scoped_lock event_lock(m_event_guard);
*/

void ReaderFF::play_impl() { m_continue_reading.store(true); }

void ReaderFF::pause_impl() { m_continue_reading.store(false); }

void ReaderFF::stop_impl()
{
    m_continue_reading.store(false);
    m_need_stop.store(true);
}

void ReaderFF::step_forward_impl()
{
    if (m_continue_reading)
    {
        STEP_LOG(L_WARN, "Can't step_forward_impl: reading not stopped!");
        return;
    }

    m_need_handle_after_force_set_pos = true;
    read_frame();
    m_need_handle_after_force_set_pos = false;
}

void ReaderFF::step_backward_impl()
{
    if (m_continue_reading)
    {
        STEP_LOG(L_WARN, "Can't step_backward_impl: reading not stopped!");
        return;
    }

    m_need_handle_after_force_set_pos = true;
    set_position_impl(get_position() - m_prev_duration);
    m_need_handle_after_force_set_pos = false;
}

void ReaderFF::rewind_forward_impl()
{
    m_need_handle_after_force_set_pos = true;
    set_position_impl(get_position() + STEP_VALUE);
    m_need_handle_after_force_set_pos = false;
}

void ReaderFF::rewind_backward_impl()
{
    m_need_handle_after_force_set_pos = true;
    set_position_impl(get_position() - STEP_VALUE);
    m_need_handle_after_force_set_pos = false;
}

void ReaderFF::set_position_impl(TimestampFF pos)
{
    // Фиксим выходы за временные границы
    if (pos < 0)
    {
        STEP_LOG(L_WARN, "Set position: pos < 0, fix set to 0");
        pos = 0;
    }

    // Если выходим за конец - не делаем set_position
    // Потому что последний валидный кадр не обязан быть на метке get_duration() и мы начнем читать невалидные кадры
    if (pos >= get_duration())
    {
        STEP_LOG(L_WARN, "Can't set position: over the end!");
        return;
    }

    STEP_LOG(L_DEBUG, "Try to set position {} to {}", get_position(), pos);

    // Отключаем обзерверов, чтобы не летели кадры и статусы
    m_frame_observers.disable();
    m_reader_observers.disable();

    // Запомнили состояние до смещения позиции
    const auto prev_state = m_state;

    // TODO Краевые условия

    // Делаем seek на нужную позицию
    seek(pos);

    if (m_state == ReaderState::Error)
    {
        STEP_LOG(L_ERROR, "Unsuccsessful seek to {}", pos);
        return;
    }

    // После seek мы стоим на ключевом кадре, надо дочитать до нужного места, если требуется
    // Читаем по одному кадру, пока следующий для чтения кадр не будет нужным
    auto next_frame_ts = m_stream->get_position();
    // duration следующего кадра: m_stream->get_pkt_duration();
    auto after_next_frame_duration = m_stream->get_pkt_duration();
    auto after_next_frame_ts = next_frame_ts + after_next_frame_duration;

    // Проверяем, что дошли по таймштампам до нужного кадра (следующий при чтении - нужный)
    const auto check_ts = [&pos, &next_frame_ts, &after_next_frame_ts]() {
        // TODO Разобраться с погрешностью
        const bool is_target_frame = next_frame_ts <= pos && after_next_frame_ts > pos;
        return !is_target_frame;
    };

    m_skip_frame_handle = true;

    while (!need_break_reading() && check_ts())
    {
        read_frame();
        next_frame_ts = m_stream->get_position();

        // Если не буферизированного пакета, то вытащить duration =из стрима не получится,
        // используем кешированное значение, но будет копиться погрешность
        auto after_next_frame_duration_from_stream = m_stream->get_pkt_duration();
        if (after_next_frame_duration_from_stream != AV_NOPTS_VALUE)
            after_next_frame_duration = after_next_frame_duration_from_stream;

        after_next_frame_ts = next_frame_ts + after_next_frame_duration;
    }

    // Дошли до нужного места (или не дошли из-за ошибки) - включаем обзерверов
    m_frame_observers.enable();
    m_reader_observers.enable();

    if (need_break_reading())
    {
        STEP_LOG(L_DEBUG, "Invalid set_position to {} because of error: state: {}, counter: {}", pos,
                 step::utils::to_string(m_state), m_invalid_counter);
        // перезапишем состояние, чтобы оно улетело подписчикам
        set_reader_state(m_state);
        return;
    }

    // Читаем нужный нам кадр
    STEP_LOG(L_DEBUG, "Try read frame with desired position, stream pos {}", m_stream->get_position());
    // TODO красиво обрабатывать проверку необходимости handle кадра
    m_skip_frame_handle = false;
    read_frame();

    STEP_LOG(L_DEBUG, "Position has been setted to {} [{}]", m_last_valid_frame->ts.count(), pos);
    set_reader_state(prev_state);
}

}  // namespace step::video::ff