#include "ff_data_packet.hpp"

#include <core/exception/assert.hpp>

#include <video/ffmpeg/utils/ff_utils.hpp>

#include <utility>

namespace step::video::ff {

FFDataPacket::BlobPacket::BlobPacket(AVBufferRef* buf_ref, size_t size, uint8_t* data)
    : m_buffer_ref(av_buffer_ref(buf_ref)), m_size(size), m_data(data)
{
    if (!m_buffer_ref)
        STEP_THROW_RUNTIME("Unable to allocate memory for buffer: {}", sizeof(AVBufferRef));
}

FFDataPacket::BlobPacket::~BlobPacket() { av_buffer_unref(&m_buffer_ref); }

std::shared_ptr<IDataPacket> FFDataPacket::create(AVPacket* packet, MediaType type, TimestampFF pts, TimestampFF dts,
                                                  TimeFF duration)
{
    return std::shared_ptr<IDataPacket>(new FFDataPacket(packet, type, pts, dts, duration));
}

FFDataPacket::FFDataPacket(AVPacket* packet, MediaType type, TimestampFF pts, TimestampFF dts, TimeFF duration)
    : m_packet(packet), m_media_type(type), m_pts(pts), m_dts(dts), m_duration(duration)
{
    if (!m_packet->buf)
    {
        // It's pure magic!
        // According to the ffmpeg documentation, AVPacket without buffer is not ref counted.
        // Most likely (in case of encoders) it will be internal memory pool based on AVCodecInternal::byte_buffer.
        // See ff_alloc_packet2(), av_fast_padded_malloc() and ff_fast_malloc() for the details.
        // So, old avcodec_encode_video2() has similar part for such packets, but in avcodec_receive_packet() there is
        // nothing looks alike. Nevertheless, actual ffmpeg.c does't care about this and just send packets directly to the muxer.
        // But according to avlib code, AVCodecInternal::byte_buffer can be reallocated at any time and for sure is freeing at closing
        // AVCodecContext. So, the right decision is make such packets truly ref counted without connection to encoder memory pool.
        // The solution was testes on any memory leaks. We don't have to keep pointer from memory pool, it is freed automaticaly inside
        // encoder. Besides that, avpacket.c has no any sings of freeing AVPacket::data at all, without AVBuffer the creator is responsible
        // for this memory.

        int ret = av_packet_make_refcounted(m_packet);
        if (ret < 0)
            STEP_THROW_RUNTIME("Error in av_packet_make_refcounted: {}", av_make_error(ret));
    }

    m_blob = std::make_shared<BlobPacket>(m_packet->buf, m_packet->size, m_packet->data);
}

FFDataPacket::FFDataPacket(const FFDataPacket& rhs)
    : m_packet(copy_packet(rhs.m_packet))
    , m_media_type(rhs.m_media_type)
    , m_pts(rhs.m_pts)
    , m_dts(rhs.m_dts)
    , m_duration(rhs.m_duration)
    , m_blob(rhs.m_blob)  // deep copy?
{
}

FFDataPacket& FFDataPacket::operator=(const FFDataPacket& rhs)
{
    FFDataPacket tmp(rhs);
    swap(*this, tmp);
    return *this;
}

FFDataPacket::FFDataPacket(FFDataPacket&& rhs)
    : m_packet(std::move(rhs.m_packet))
    , m_media_type(std::move(rhs.m_media_type))
    , m_pts(std::move(rhs.m_pts))
    , m_dts(std::move(rhs.m_dts))
    , m_duration(std::move(rhs.m_duration))
    , m_blob(std::move(rhs.m_blob))
{
}

FFDataPacket& FFDataPacket::operator=(FFDataPacket&& rhs)
{
    FFDataPacket tmp(std::move(rhs));
    swap(*this, tmp);
    return *this;
}

FFDataPacket::~FFDataPacket()
{
    av_packet_unref(m_packet);
    av_free(m_packet);
}

// std::shared_ptr<IDataPacket> FFDataPacket::clone() const
// {
//     return std::make_shared<IDataPacket>(new FFDataPacket(*this));
// }

}  // namespace step::video::ff