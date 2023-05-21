#pragma once

#include "decoder_video.hpp"
#include "demuxer_queue.hpp"
#include "stream.hpp"

#include <video/ffmpeg/interfaces/stream_reader.hpp>

#include <mutex>

namespace step::video::ff {

class StreamReader;

class DemuxedStream : public IStream, public std::enable_shared_from_this<DemuxedStream>
{
public:
    DemuxedStream(std::shared_ptr<StreamReader> reader, StreamId stream_id);
    virtual ~DemuxedStream();

    TimeFF get_duration() const override;
    TimestampFF get_position() override;
    MediaType get_media_type() const override;
    //SP<const Conf::IFormatCodec> GetFormatCodec() const;
    DataPacketPtr read() override;
    void request_seek(TimestampFF time, const StreamPtr& result_checker) override;
    void do_seek() override;
    bool get_seek_result() override;
    void terminate() override;
    bool is_terminated() const override;
    void release_internal_data() override;

    FramePtr read_frame() override;  // *STEP

    void unlink_from_reader();

private:
    std::shared_ptr<StreamReader> m_reader;
    StreamId m_stream;
    TimestampFF m_position;       ///< Позиция последнего прочитанного пакета
    std::mutex m_seek_mutex;      ///< Для блокировки seek
    std::mutex m_read_mutex;      ///< Для блокировки read
    TimestampFF m_seek_position;  ///< Метка, на которую нужно выполнить Seek
                                  //    bool            m_lastSeekResult;		///< Результат Seek
    std::shared_ptr<IDataPacket> m_buffered_packet;  ///< Буферизированный пакет для проверки Seek
    std::atomic_bool m_terminated;  ///< флаг для принудительной остановки конвертации
    TimeFF m_working_time;          ///< Счетчик времени
    size_t m_processed_count;  ///< Количество обработанных объектов

    // TODO Decoder interface for audio *STEP
    std::unique_ptr<DecoderVideoFF> m_video_decoder;  // *STEP
    FramePtr m_buffered_data{nullptr};                // *STEP
};

class StreamReader : public IStreamReader, public std::enable_shared_from_this<StreamReader>
{
    friend DemuxedStream;

protected:
    /// Вспомогательный класс для связывания Raw-потока с Reader-ом
    class StreamInfo
    {
    private:
        DemuxedStream* m_demuxed_stream;  ///< сырой указатель, чтобы не наращивать счетчик ссылок
        TimestampFF m_seek_time;  ///< позиция, на которую был запрошен Seek для данного потока
        std::vector<StreamPtr>
            m_seek_result_checkers;  ///< список объектов, у которых надо проверить результат позиционирования
        bool m_seek_already_done;    ///< признак, что Seek был вызван для этого потока

    public:
        StreamInfo();
        ~StreamInfo();

        DemuxedStream* get_stream() const;       ///< Возврат сырого указателя на raw-поток
        void set_stream(DemuxedStream* stream);  ///< Установка сырого указателя на raw-поток
        bool is_stream_enabled() const;  ///< Включен ли поток (указатель на raw - не NULL?)
        void unlink_from_reader();       ///< Отсоединение raw-потока от Reader-a

        /* clang-format off */
        void request_seek(TimestampFF time, StreamPtr pResultChecker); ///< Установка времени, на которое надо спозиционироваться и callback-а для проверки правильности позиционирования
        bool get_seek_result(TimestampFF seekTime);  ///< Вызов всех callback-ов, которые проверяют правильность позиционивания. seekTime - реальная точка позиционирования с учетом отступов назад.
        TimestampFF get_seek_position() const;  ///< Получение минимальной временной метки (из всех, которые пришли), на которую надо спозиционироваться
        bool seek_already_done() const;  ///< Признак сделал ли уже Seek для данного потока
        void reset_state_before_seek();  ///< Сброс состояния перед Seek
        void reset_state_after_seek();   ///< Сброс состояния после Seek
        /* clang-format on */
    };

    DemuxerPtr m_demuxer;
    std::vector<StreamInfo> m_streams;

private:
    std::mutex m_seek_mutex;

public:
    static std::shared_ptr<IStreamReader> create(const DemuxerPtr& demuxer);

    StreamReader(const DemuxerPtr&);
    virtual ~StreamReader() = default;

public:
    TimeFF get_duration() const override;
    StreamPtr get_stream(StreamId stream_id) override;
    void release_stream(StreamId stream_id) override;
    int get_stream_count() const override;
    int get_active_stream_count() const override;
    FormatCodec get_format_codec(StreamId stream_id) const override;  // *STEP

    void release_internal_data(StreamId stream_id);

protected:
    void request_seek(StreamId stream_id, TimestampFF time, const StreamPtr& result_checker);
    void seek(StreamId stream_id);
};

}  // namespace step::video::ff