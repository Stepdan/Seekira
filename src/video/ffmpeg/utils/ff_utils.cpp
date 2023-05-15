#include "ff_utils.hpp"
#include "ff_types.hpp"

#include <core/exception/assert.hpp>

extern "C" {
#include <libavutil/mem.h>
#include <libavutil/error.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
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

int read_frame_fixed(AVFormatContext* ctx, AVPacket* pkt)
{
    int read_count = 0;
    TimestampFF cur_pos = AV_NOPTS_VALUE;
    TimestampFF total_size = AV_NOPTS_VALUE;

    while (true)
    {
        if (ctx->pb && ctx->pb->pos != AV_NOPTS_VALUE)
            cur_pos = ctx->pb->pos;

        int res = av_read_frame(ctx, pkt);
        if (res < 0)
        {
            // Чтение починить не удастся, выходим
            /* clang-format off */
            if (false 
                || res == AVERROR(ENOENT)  // No such file or directory
                || res == AVERROR(ENXIO)         // No such device or address
                || res == AVERROR(ENODEV)        // No such device
                || res == AVERROR_EOF)
            {
                return res;
            }
            /* clang-format on */

            if (res == AVERROR(EAGAIN) && read_count < 20)  // (-11)
            {
                // эта заглушка нужна для некоторых форматов (например, asf и VOB)
                // они глючили после Seek, но после прочтения нескольких пакетов все становилось нормально
                ++read_count;
                continue;
            }

            ///@todo разобаться, в каких случаях AVERROR(?) имеет смысл это исправление, применять его только в таких случаях,
            ///перечисление ошибок выше - выпилить
            if (cur_pos != AV_NOPTS_VALUE && cur_pos > 0)
            {
                if (total_size == AV_NOPTS_VALUE)
                {
                    total_size = avio_size(ctx->pb);
                    if (total_size < 0)
                    {
                        total_size = avio_tell(ctx->pb);
                        if (total_size < 0)
                            total_size = AV_NOPTS_VALUE;
                    }
                }
                if (total_size > 0)
                {
                    if (total_size - cur_pos > 102400)
                    {
                        /// мы находимся не в конце файла
                        read_count = 0;
                        cur_pos += 1024;
                        if (av_seek_frame(ctx, -1, cur_pos, AVSEEK_FLAG_BYTE) < 0)
                        {
                            /// Все таки мы находимся в конце файла, либо seek невозможен и мы уже ничего сделать не можем
                            return res;
                        }

                        continue;
                    }
                }
            }
        }

        return res;
    }
}

int64_t stream_to_global(int64_t stream_time, const AVRational& stream_time_base)
{
    if (stream_time == AV_NOPTS_VALUE)
        return AV_NOPTS_VALUE;

    return av_rescale_q(stream_time, stream_time_base, TIME_BASE_Q);
}

int64_t global_to_stream(int64_t global_time, const AVRational& stream_time_base)
{
    if (global_time == AV_NOPTS_VALUE)
        return AV_NOPTS_VALUE;

    return av_rescale_q(global_time, TIME_BASE_Q,
                        stream_time_base);  // res = global_time * TIME_BASE_Q / stream->time_base
}

}  // namespace step::video::ff