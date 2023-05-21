#pragma once

#include "parser.hpp"
#include "packet_queue.hpp"

#include <video/ffmpeg/interfaces/demuxer.hpp>

namespace step::video::ff {

class DemuxerQueue : public IDemuxer
{
public:
    DemuxerQueue(const std::shared_ptr<ParserFF>& parser);
    ~DemuxerQueue();

    TimeFF get_duration() const override;
    TimeFF get_stream_duration(StreamId stream) const override;

    int get_stream_count() const override;
    MediaType get_stream_type(StreamId index) const override;

    void enable_stream(StreamId stream, bool value) override;

    TimestampFF seek(TimestampFF time) override;
    bool is_eof_reached() override;
    std::shared_ptr<IDataPacket> read(StreamId stream) override;
    void release_internal_data(StreamId stream) override;

    FormatCodec get_format_codec(StreamId) const override;  // *STEP

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