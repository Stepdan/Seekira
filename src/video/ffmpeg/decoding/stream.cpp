#include "stream_reader.hpp"

#include <core/log/log.hpp>

namespace step::video::ff {

DemuxedStream::DemuxedStream(std::shared_ptr<StreamReader> reader, StreamId stream_id)
    : m_reader(reader)
    , m_stream(stream_id)
    , m_position(0)
    , m_seek_position(0)
    , m_terminated(false)
    , m_working_time(0)
    , m_processed_count(0)
{
}

DemuxedStream::~DemuxedStream() { unlink_from_reader(); }

void DemuxedStream::unlink_from_reader()
{
    if (!m_reader)
        return;

    std::unique_lock<std::mutex> lock(m_read_mutex);
    std::shared_ptr<StreamReader> reader_tmp = m_reader;
    m_reader
        .reset();  // обнуляем именно тут, чтобы не было рекурсивного зацикливания при вызове m_reader->ReleaseStream(..)
    reader_tmp->release_stream(m_stream);
    m_position = 0;
}

TimeFF DemuxedStream::get_duration() const
{
    if (!m_reader)
        STEP_THROW_RUNTIME("Reader is NULL");

    return m_reader->m_demuxer->get_stream_duration(m_stream);
}

TimestampFF DemuxedStream::get_position()
{
    if (!m_reader)
        STEP_THROW_RUNTIME("Reader is NULL");

    return m_position;
}

MediaType DemuxedStream::get_media_type() const
{
    if (!m_reader)
        STEP_THROW_RUNTIME("Reader is NULL");

    return m_reader->m_demuxer->get_stream_type(m_stream);
}

//SP<const Conf::IFormatCodec> GetFormatCodec() const;

DataPacketPtr DemuxedStream::read()
{
    if (!m_reader)
        STEP_THROW_RUNTIME("Reader is NULL");

    if (m_terminated)
        return nullptr;

    std::unique_lock lock(m_read_mutex);
    DataPacketPtr data;
    if (m_buffered_packet)
    {
        data.swap(m_buffered_packet);
    }
    else
    {
        data = m_reader->m_demuxer->read(m_stream);
    }

    if (data)
    {
        m_position = data->pts_or_dts();
        if (m_position != AV_NOPTS_VALUE)
            m_position += data->duration();

        ++m_processed_count;
    }

    return data;
}

void DemuxedStream::request_seek(TimestampFF time, const StreamPtr& result_checker)
{
    if (!m_reader)
        STEP_THROW_RUNTIME("Reader is NULL");

    m_terminated.store(false);
    m_seek_position = time;
    m_buffered_packet.reset();
    m_position = AV_NOPTS_VALUE;

    // сначала добавляем обработчик для декодеров
    if (result_checker)
        m_reader->request_seek(m_stream, time, result_checker);

    // потом добавляем обработчик для сжатых пакетов
    m_reader->request_seek(m_stream, time, shared_from_this());  // leak?
}

void DemuxedStream::do_seek()
{
    if (!m_reader)
        STEP_THROW_RUNTIME("Reader is NULL");

    std::unique_lock lock(m_seek_mutex, std::try_to_lock);
    if (!lock)
    {
        STEP_LOG(L_DEBUG, "DemuxedStream {} already in seek mode", m_stream);
        return;
    }

    m_reader->seek(m_stream);
}

bool DemuxedStream::get_seek_result()
{
    std::unique_lock lock(m_read_mutex);
    /// всегда читаем новый пакет после Seek, иначе подбор нужной позиции не работает
    while (true)
    {
        m_buffered_packet = m_reader->m_demuxer->read(m_stream);
        if (!m_buffered_packet || m_seek_position <= 0)
        {
            break;
        }

        if (m_buffered_packet->is_key_frame())
        {
            /// когда m_seek_position > 0, то пакет должен быть ключевым
            break;
        }
    }

    m_position = m_buffered_packet ? m_buffered_packet->pts_or_dts() : AV_NOPTS_VALUE;
    if (!m_buffered_packet)
        return false;

    bool res = (m_position <= m_seek_position) && (m_buffered_packet->is_key_frame());
    STEP_LOG(L_DEBUG, "DemuxedStream {}: pos={}, key_frame={} is {}", m_stream, m_position,
             m_buffered_packet->is_key_frame(), (res ? "ok" : "error"));
    return res;
}

void DemuxedStream::terminate() { m_terminated.store(true); }

bool DemuxedStream::is_terminated() const { return m_terminated; }

void DemuxedStream::release_internal_data()
{
    std::unique_lock lock(m_read_mutex);
    m_buffered_packet.reset();
    m_position = AV_NOPTS_VALUE;
    m_reader->release_internal_data(m_stream);
}

}  // namespace step::video::ff