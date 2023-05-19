#pragma once

#include "types_safe.hpp"
#include "data_packet.hpp"

#include "video/frame/interfaces/frame.hpp"

#include <video/ffmpeg/utils/types.hpp>

#include <atomic>
#include <queue>
#include <ratio>

namespace step::video::ff {

class DecoderVideoFF
{
public:
    struct Initializer
    {
        AVCodecID codec_id;
        uint32_t codec_tag;
        FrameSize frame_size;
        AVRational fps;
        bool image_flag{false};
        // TODO AVCodecParameters
    };

public:
    DecoderVideoFF();
    ~DecoderVideoFF();

    bool open(const Initializer& init);
    void flush(TimestampFF start_time);
    void release_internal_data();
    FramePtr decode(const std::shared_ptr<IDataPacket>& data);

private:
    FramePtr decode_internal(const std::shared_ptr<IDataPacket>& data);
    AVRational get_fps() const;
    FramePtr get_next_queued_frame();

private:
    /* clang-format off */
    DecoderContextSafe m_codec;
    AVDictionary*      m_options                 = { nullptr           }; // decoder parameters
    bool               m_can_reopen_decoder      = { false             };
    bool               m_clock_reset             = { false             }; // clock have to be reseted after seek
    TimestampFF        m_clock                   = { AV_NOPTS_VALUE    }; // time point of last outgoing packet
    FrameSize          m_out_frame_size          = { 0, 0              };
    bool               m_image_flag              = { false             };
    bool               m_first_frame_after_seek  = { false             };
    bool               m_use_dts                 = { false             };
    TimeFF             m_prev_duration_clock     = { 0                 }; // Length of last frame. Used to calc next time point in case of mistake
    TimeFF             m_prev_duration_frame_pkt = { 0                 }; // The duration of the packet that the decoder for the output frame reported.
    SwsContextSafe     m_sws_context                                    ;
    PixFmt             m_best_pix_fmt            = { PixFmt::Undefined };
    AVPixelFormat      m_open_pix_fmt            = { AV_PIX_FMT_NONE   }; // pixel format used for Open
    std::string        m_codec_name                                     ;
    AVCodecID          m_codec_id                                       ;
    AVRational         m_fps                                            ;
    uint32_t           m_codec_tag               = { 0                 }; 

    std::pair<int, int> m_dpi; // Physical image characteristics, resolution in pixels per inch.    

    std::queue<FramePtr> m_queue;
    FramePtr m_prev_frame;

    TimestampFF m_start_time {0}; /// начальная временная метка, выдаваемая декодером
    TimestampFF m_end_time{AV_NOPTS_VALUE};   /// конечная временная метка, выдаваемая декодером

    size_t m_processed_count = { 0 };
    TimestampFF m_all_time       = { 0 };
    TimestampFF m_csc_time       = { 0 };
    TimestampFF m_ff_time        = { 0 };
    TimestampFF m_ref_time       = { 0 };
    /* clang-format on */
};

}  // namespace step::video::ff