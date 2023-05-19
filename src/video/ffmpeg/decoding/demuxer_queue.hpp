#pragma once

#include "parser.hpp"
#include "packet_queue.hpp"

namespace step::video::ff {

class DemuxerQueue
{
public:
    DemuxerQueue(const std::shared_ptr<ParserFF>& parser);
    ~DemuxerQueue();

    TimeFF get_duration() const;
    TimeFF get_stream_duration(StreamId stream) const;

    int get_stream_count() const;
    MediaType get_stream_type(StreamId index) const;

    void enable_stream(StreamId stream, bool value);

    TimestampFF seek(TimestampFF time);
    bool is_eof_reached();
    std::shared_ptr<IDataPacket> read(StreamId stream);
    void release_internal_data(StreamId stream);

private:
    void close();
    void reset_queue();
    bool check_streams_position(TimestampFF max_allowed_position);
    bool fill_queue(bool key_packets_must_exist);
    bool read_packets_from_parser(int packet_count);
    bool seek_internal(TimestampFF time);

private:
    std::shared_ptr<ParserFF> m_parser;
    int m_streams;                  ///< кол-во потоков в файле
    bool m_key_packets_must_exist;  ///< режим заполнения очереди
    TimestampFF m_seek_time;        ///< время Seek
    bool m_eof_is_reached;          ///< признак конца файла
    std::recursive_mutex m_mutex;  ///< Для блокировки объекта во время изменения внутренних структур
    TimestampFF m_working_time;  ///< Счетчик времени
    size_t m_processed_count;    ///< Количество обработанных объектов

    std::vector<std::shared_ptr<PacketQueue>> m_queues;
};

}  // namespace step::video::ff