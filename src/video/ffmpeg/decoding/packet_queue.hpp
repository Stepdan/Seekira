#pragma once

#include <video/ffmpeg/utils/types.hpp>

#include <list>
#include <memory>
#include <mutex>

namespace step::video::ff {

class IDataPacket;

class PacketQueue
{
public:
    PacketQueue(MediaType type);
    ~PacketQueue();

    void push(const std::shared_ptr<IDataPacket>& packet);
    std::shared_ptr<IDataPacket> pop();
    TimestampFF position() const;  ///< Текущая позиция очереди (временная метка первого пакета)
    int size() const;
    void reset();
    bool is_enabled() const;
    void set_enabled(bool enabled);
    bool is_full() const;
    bool can_be_empty() const;
    int get_key_frame_count() const;

protected:
    bool m_enabled;       ///< вкл/выкл потока
    bool m_can_be_empty;  ///< = true для субтитров
    int m_key_count;      ///< кол-во ключевых пакетов в очереди
    int64_t m_current_size;  ///< @todo сейчас не используется: текущий размер очереди
    unsigned int m_max_packets;  ///< @todo сейчас не используется: максимально допустимое кол-во пакетов
    int64_t m_max_size;  ///< @todo сейчас не используется: максимально допустимый суммарный размер пакетов
    std::list<std::shared_ptr<IDataPacket>> m_list;  ///< Очередь с пакетами для конкретного потока
    mutable std::recursive_mutex m_mutex;  ///< Для блокировки объекта во время изменения внутренних структур
};

}  // namespace step::video::ff