#include "ff_utils.hpp"

#include <core/exception/assert.hpp>

extern "C" {
#include <libavutil/mem.h>
}

namespace step::video::ff {

AVPacket* create_packet(size_t size)
{
    AVPacket* packet;

    if (size > 0)
    {
        int ret = 0;
        packet = (AVPacket*)av_malloc(sizeof(AVPacket));
        if (!packet)
            STEP_THROW_RUNTIME("Unable to allocate memory for AVPacket");
        ret = av_new_packet(packet, static_cast<int>(size));
        if (ret == 0)
            return packet;
        av_free(packet);
        STEP_THROW_RUNTIME("Unable to allocate memory for AVPacket");
    }
    else
    {
        packet = av_packet_alloc();
        packet->data = nullptr;
        packet->size = 0;
        return packet;
    }
}

AVPacket* copy_packet(const AVPacket* src)
{
    if (!src)
        return nullptr;

    AVPacket* dst = create_packet(0);
    if (!dst)
        STEP_THROW_RUNTIME("Unable to allocate memory for AVPacket");

    if (av_packet_ref(dst, src) < 0)
        STEP_THROW_RUNTIME("Unable to duplicate packet");

    return dst;
}

void release_packet(AVPacket** packet)
{
    if (packet && *packet)
    {
        av_packet_unref(*packet);
        av_free(*packet);
        *packet = nullptr;
    }
}

}  // namespace step::video::ff