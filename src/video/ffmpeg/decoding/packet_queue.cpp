#include "packet_queue.hpp"
#include "data_packet.hpp"

namespace step::video::ff {

PacketQueue::PacketQueue(MediaType type)
{
    m_current_size = 0;
    m_max_packets = 999999;
    m_max_size = 50 * 1000000;
    m_enabled = true;
    m_can_be_empty = (type != MediaType::Video && type != MediaType::Audio);
    m_key_count = 0;
}

void PacketQueue::push(const std::shared_ptr<IDataPacket>& packet)
{
    const int size = packet->size();
    const bool isKeyFrame = packet->is_key_frame();
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    m_list.push_back(packet);
    m_current_size += size;
    m_key_count += isKeyFrame;
}

std::shared_ptr<IDataPacket> PacketQueue::pop()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    std::shared_ptr<IDataPacket> packet = m_list.front();
    const int size = packet->size();
    const bool is_key_frame = packet->is_key_frame();
    m_current_size -= size;
    m_key_count -= is_key_frame;
    m_list.pop_front();
    return packet;
}

TimestampFF PacketQueue::position() const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    if (m_current_size <= 0)
        return AV_NOPTS_VALUE;

    for (auto it = m_list.begin(); it != m_list.end(); ++it)
    {
        TimestampFF pos = (*it)->pts_or_dts();
        if (pos != AV_NOPTS_VALUE)
            return pos;
    }
    //assert(0);
    return AV_NOPTS_VALUE;
}

int PacketQueue::size() const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    return static_cast<int>(m_list.size());
}

void PacketQueue::reset()
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    std::list<std::shared_ptr<IDataPacket>>().swap(m_list);
    m_current_size = 0;
    m_key_count = 0;
}

bool PacketQueue::is_enabled() const { return m_enabled; }

void PacketQueue::set_enabled(bool enabled)
{
    m_enabled = enabled;
    if (!m_enabled)
    {
        reset();
    }
}

bool PacketQueue::is_full() const
{
    std::unique_lock<std::recursive_mutex> lock(m_mutex);
    return (m_list.size() > m_max_packets) || (m_current_size > m_max_size);
}

bool PacketQueue::can_be_empty() const { return m_can_be_empty; }

int PacketQueue::get_key_frame_count() const { return m_key_count; }

PacketQueue::~PacketQueue() { reset(); }

}  // namespace step::video::ff