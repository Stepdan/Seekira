#pragma once

#include <core/exception/assert.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <fmt/format.h>

#include <memory>

namespace step::video::ff {

namespace details {
/* clang-format off */
struct DeleterEmpty           { using ptr_type = void*                ; void operator()(ptr_type &ptr) { ptr = nullptr; } };
struct DeleterAVFreeP         { using ptr_type = void*                ; void operator()(ptr_type &ptr) { if(!ptr) return; av_freep(&ptr); } };
struct DeleterSwsFreeContext  { using ptr_type = SwsContext*          ; void operator()(ptr_type &ptr) { if(!ptr) return; sws_freeContext(ptr); ptr = nullptr; } };
//struct DeleterAVFifoFree      { using ptr_type = AVFifoBuffer*        ; void operator()(ptr_type &ptr) { if(!ptr) return; av_fifo_free(ptr); ptr = nullptr; } };
struct DeleterAVDictFree      { using ptr_type = AVDictionary*        ; void operator()(ptr_type &ptr) { if(!ptr) return; av_dict_free(&ptr); ptr = nullptr; } };
struct DeleterAVCloseInput    { using ptr_type = AVFormatContext*     ; void operator()(ptr_type &ptr) { if(!ptr) return; avformat_close_input(&ptr); ptr = nullptr; } };
struct DeleterAVFreeContext   { using ptr_type = AVFormatContext*     ; void operator()(ptr_type &ptr) { if(!ptr) return; avformat_free_context(ptr); ptr = nullptr; } };
struct DeleterAVFreeCodecCtx  { using ptr_type = AVCodecContext*      ; void operator()(ptr_type ptr)  { avcodec_free_context(&ptr); } };
struct DeleterAVFrameFree     { using ptr_type = AVFrame*             ; void operator()(ptr_type &ptr) { if(!ptr) return; av_frame_free(&ptr); ptr = nullptr; } };
struct DeleterAVFrameUnref    { using ptr_type = AVFrame*             ; void operator()(ptr_type &ptr) { if(!ptr) return; av_frame_unref(ptr); ptr = nullptr; } };
struct DeleterAVPacketUnref   { using ptr_type = AVPacket*            ; void operator()(ptr_type &ptr) { if(!ptr) return; av_packet_unref(ptr); ptr = nullptr; } };
//struct DeleterAVBSF           { using ptr_type = AVBSFContext*        ; void operator()(ptr_type &ptr) { if(!ptr) return; av_bsf_free(&ptr); ptr = nullptr; } };
struct DeleterAVCodecPar      { using ptr_type = AVCodecParameters*   ; void operator()(ptr_type &ptr) { if(!ptr) return; avcodec_parameters_free(&ptr); ptr = nullptr; } };
/* clang-format on */
}  // namespace details

class SwsContextSafe : public std::unique_ptr<SwsContext, details::DeleterSwsFreeContext>
{
public:
    struct SwsItem
    {
        int width, height;
        AVPixelFormat pix_fmt;
        SwsFilter* filter;
        SwsItem(int w, int h, AVPixelFormat fmt, SwsFilter* f) : width(w), height(h), pix_fmt(fmt), filter(f) {}
    };

    /* clang-format off */
    SwsContextSafe() = default;
    SwsContextSafe(int src_width, int src_height, AVPixelFormat src_pix_fmt,
                   int dst_width, int dst_height, AVPixelFormat dst_pix_fmt,
                   int flags = SWS_FAST_BILINEAR, SwsFilter* src_filter = nullptr,
                   SwsFilter* dst_filter = nullptr, const double* param = nullptr
    ) : std::unique_ptr<SwsContext, details::DeleterSwsFreeContext>(
        sws_getContext(src_width, src_height, src_pix_fmt,
                       dst_width, dst_height, dst_pix_fmt,
                       (src_width == dst_width) && (src_height == dst_height) ? SWS_POINT : flags,
                       src_filter, dst_filter, param))
    {
        if(!this->get())
            STEP_THROW_RUNTIME("Invalid SwsContext creation!");
    }
    /* clang-format on */
};

class FormatContextSafe : public std::unique_ptr<AVFormatContext, details::DeleterAVFreeContext>
{
public:
    FormatContextSafe(AVFormatContext* ctx = nullptr)
        : std::unique_ptr<AVFormatContext, details::DeleterAVFreeContext>(ctx)
    {
        if (!this->get())
            return;

        FormatContextSafe tmp(avformat_alloc_context());
        swap(tmp);

        if (!this->get())
            STEP_THROW_RUNTIME("Can't allocate format context!");
    }
};

class FormatContextInputSafe : public std::unique_ptr<AVFormatContext, details::DeleterAVCloseInput>
{
    FormatContextInputSafe(AVFormatContext* ctx = nullptr)
        : std::unique_ptr<AVFormatContext, details::DeleterAVCloseInput>(ctx)
    {
    }
    FormatContextInputSafe(const std::string& filename, AVInputFormat* input_format, AVDictionary** options)
        : std::unique_ptr<AVFormatContext, details::DeleterAVCloseInput>(avformat_alloc_context())
    {
        if (!this->get())
            STEP_THROW_RUNTIME("Can't allocate format context input: {}", filename);

        auto* ptr = this->get();
        int errorCode = avformat_open_input(&ptr, filename.c_str(), input_format, options);
        if (errorCode < 0)
            STEP_THROW_RUNTIME("Can't open (avformat_open_input) file {}: unsupported format or file is corrupted",
                               filename);
    }
};

class CodecSafe : public std::unique_ptr<AVCodec, details::DeleterEmpty>
{
public:
    CodecSafe() {}
    CodecSafe(const std::string& codec_name)
        : std::unique_ptr<AVCodec, details::DeleterEmpty>(
              const_cast<AVCodec*>(avcodec_find_decoder_by_name(codec_name.c_str())))
    {
        if (!this->get())
            STEP_THROW_RUNTIME("Can't find decoder by name");
    }
    CodecSafe(AVCodecID codec_id)
        : std::unique_ptr<AVCodec, details::DeleterEmpty>(const_cast<AVCodec*>(avcodec_find_decoder(codec_id)))
    {
        if (!this->get())
            STEP_THROW_RUNTIME("Can't find decoder by id");
    }
};

class CodecContextSafe : public std::unique_ptr<AVCodecContext, details::DeleterAVFreeCodecCtx>
{
public:
    CodecContextSafe(AVCodecParameters* par = nullptr)
        : std::unique_ptr<AVCodecContext, details::DeleterAVFreeCodecCtx>([par]() {
            AVCodecContext* ctx = avcodec_alloc_context3(nullptr);
            if (!ctx)
                STEP_THROW_RUNTIME("Can't create empty codec context");

            if (par && avcodec_parameters_to_context(ctx, par) < 0)
                STEP_THROW_RUNTIME("Can't fill codec context params");

            return ctx;
        }())
    {
    }

    CodecContextSafe(const AVCodec* codec)
        : std::unique_ptr<AVCodecContext, details::DeleterAVFreeCodecCtx>(avcodec_alloc_context3(codec))
    {
        if (!this->get())
            STEP_THROW_RUNTIME("Can't create codec context");
    }

    int open(const AVCodec* codec, AVDictionary** options = nullptr) noexcept
    {
        return avcodec_open2(this->get(), codec, options);
    }

    int close() { return avcodec_close(this->get()); }
};

class DecoderContextSafe : CodecContextSafe
{
public:
    using CodecContextSafe::CodecContextSafe;

    int open_decoder(AVCodecID id, AVDictionary** options = nullptr) noexcept
    {
        return this->open(avcodec_find_decoder(id), options);
    }

    int send_packet(const AVPacket* pkt) noexcept { return avcodec_send_packet(this->get(), pkt); }

    int recieve_frame(AVFrame* frame) noexcept { return avcodec_receive_frame(this->get(), frame); }
};

class FrameSafe : public std::unique_ptr<AVFrame, details::DeleterAVFrameFree>
{
public:
    FrameSafe(AVFrame* data = nullptr)
        : std::unique_ptr<AVFrame, details::DeleterAVFrameFree>(data ? data : av_frame_alloc())
        , m_unref_only(!!data)
        , m_free_data(false)
    {
        if (!this->get())
            STEP_THROW_RUNTIME("Unable to allocate memory for AVFrame struct: sizeof {}", sizeof(AVFrame));
    }

    FrameSafe(AVPixelFormat fmt, int width, int height, int aspect_x = 1, int aspect_y = 1)
        : std::unique_ptr<AVFrame, details::DeleterAVFrameFree>(av_frame_alloc())
        , m_unref_only(false)
        , m_free_data(true)
    {
        STEP_UNDEFINED("Undefined AVFrame creation!");
        // if (!this->get())
        //     STEP_THROW_RUNTIME("Unable to allocate memory for AVFrame struct: sizeof {}", sizeof(AVFrame));

        // auto size = av_image_alloc(this->get()->data, this->get()->linesize, width, height, fmt, 32);
        // if (size < 0)
        // {
        // 	size = av_image_get_buffer_size(fmt, width, height, 32);
        // 	MOVAVI_EXCEPTION_ASSERT(Movavi::Core::MemoryException("Unable to allocate memory for AVFrame data", size)
        // 		<< Movavi::Core::MemoryException::FrameWidth(width)
        // 		<< Movavi::Core::MemoryException::FrameWidth(height));
        // }
        // this->get()->format = fmt;
        // this->get()->width = width;
        // this->get()->height = height;
        // this->get()->key_frame = 0;
        // this->get()->sample_aspect_ratio.num = aspect_x;
        // this->get()->sample_aspect_ratio.den = aspect_y;
    }

    void swap(FrameSafe& rhs)
    {
        this->swap(rhs);
        std::swap(m_unref_only, rhs.m_unref_only);
        std::swap(m_free_data, rhs.m_free_data);
    }

    AVFrame* operator()() { return get(); }

    ~FrameSafe()
    {
        if (m_unref_only)
        {
            av_frame_unref(this->get());
            reset();
        }
        if (m_free_data)
        {
            av_freep(this->get()->data);
        }
    }

private:
    bool m_unref_only;
    bool m_free_data;
};

using DictionarySafe = std::unique_ptr<AVDictionary, details::DeleterAVDictFree>;
using PacketSafe = std::unique_ptr<AVPacket, details::DeleterAVPacketUnref>;

}  // namespace step::video::ff

template <>
struct fmt::formatter<step::video::ff::SwsContextSafe::SwsItem> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::video::ff::SwsContextSafe::SwsItem& item, FormatContext& ctx)
    {
        const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(item.pixelFormat);
        return format_to(ctx.out(), "pixel format desc: {}, width: {}, height: {}", desc->name, width, height);
    }
};
