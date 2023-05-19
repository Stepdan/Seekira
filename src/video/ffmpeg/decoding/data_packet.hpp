#pragma once

#include "types.hpp"

#include <core/base/interfaces/blob.hpp>

extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/avutil.h>
}

#include <memory>

namespace step::video::ff {

class IDataPacket
{
public:
    virtual ~IDataPacket() = default;

    virtual TimeFF duration() const noexcept = 0;
    virtual void set_duration(TimeFF value) = 0;

    virtual TimestampFF pts() const noexcept = 0;
    virtual void set_pts(TimestampFF ts) = 0;

    virtual TimestampFF dts() const noexcept = 0;
    virtual void set_dts(TimestampFF ts) = 0;

    virtual TimestampFF pts_or_dts() const noexcept = 0;

    virtual MediaType get_media_type() const noexcept = 0;

    virtual StreamId get_stream_index() const = 0;
    virtual void set_stream_index(StreamId id) = 0;

    virtual bool is_key_frame() const = 0;
    virtual bool is_corrupted() const = 0;
    virtual bool is_minor_field() const = 0;

    virtual int size() const = 0;

    virtual const AVPacket* get_packet() const noexcept = 0;

    virtual std::shared_ptr<IBlob> get_data() const = 0;
};

class DataPacketFF : public IDataPacket
{
public:
    static std::shared_ptr<IDataPacket> create(AVPacket* packet, MediaType type, TimestampFF pts, TimestampFF dts,
                                               TimeFF duration);
    //std::shared_ptr<IDataPacket> clone() const;

public:
    DataPacketFF(AVPacket* packet, MediaType type, TimestampFF pts, TimestampFF dts, TimeFF duration);
    virtual ~DataPacketFF();

    DataPacketFF(const DataPacketFF& rhs);
    DataPacketFF& operator=(const DataPacketFF& rhs);
    DataPacketFF(DataPacketFF&& rhs);
    DataPacketFF& operator=(DataPacketFF&& rhs);

    friend void swap(DataPacketFF& lhs, DataPacketFF& rhs) noexcept
    {
        std::swap(lhs.m_packet, rhs.m_packet);
        std::swap(lhs.m_media_type, rhs.m_media_type);
        std::swap(lhs.m_pts, rhs.m_pts);
        std::swap(lhs.m_dts, rhs.m_dts);
        std::swap(lhs.m_duration, rhs.m_duration);
        std::swap(lhs.m_blob, rhs.m_blob);
    }

public:
    TimeFF duration() const noexcept override { return m_duration; }
    void set_duration(TimeFF value) override { m_duration = value; }

    TimestampFF pts() const noexcept override { return m_pts; }
    void set_pts(TimestampFF ts) override { m_pts = ts; }

    TimestampFF dts() const noexcept override { return m_dts; }
    void set_dts(TimestampFF ts) override { m_dts = ts; }

    TimestampFF pts_or_dts() const noexcept override { return (m_pts != AV_NOPTS_VALUE ? m_pts : m_dts); }

    MediaType get_media_type() const noexcept override { return m_media_type; }

    StreamId get_stream_index() const override { return static_cast<StreamId>(m_packet->stream_index); }
    void set_stream_index(StreamId id) override
    {
        m_packet->stream_index = static_cast<decltype(m_packet->stream_index)>(id);
    }

    bool is_key_frame() const override { return ((m_packet->flags & AV_PKT_FLAG_KEY) != 0); }
    bool is_corrupted() const override { return ((m_packet->flags & AV_PKT_FLAG_CORRUPT) != 0); }
    bool is_minor_field() const override { return m_packet->pos == -1; }

    int size() const override { return m_packet->size; }

    const AVPacket* get_packet() const noexcept override { return m_packet; }

    std::shared_ptr<IBlob> get_data() const override { return m_blob; }

private:
    class BlobPacket : public IBlob
    {
    public:
        BlobPacket(AVBufferRef* buf_ref, size_t size, uint8_t* data);
        ~BlobPacket();

        uint8_t* data() override { return m_data; }
        const uint8_t* data() const override { return m_data; }
        size_t size() const { return m_size; }
        size_t real_size() const { return m_buffer_ref->size; }

    private:
        AVBufferRef* m_buffer_ref;
        size_t m_size;    // m_packet->size
        uint8_t* m_data;  // m_packet->data
    };

private:
    AVPacket* m_packet;
    MediaType m_media_type;

    TimestampFF m_pts;
    TimestampFF m_dts;
    TimeFF m_duration;

    std::shared_ptr<IBlob> m_blob;
};

}  // namespace step::video::ff