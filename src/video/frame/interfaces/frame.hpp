#pragma once

#include "pixel_format.hpp"
#include "frame_size.hpp"

#include <base/utils/time_utils.hpp>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <limits.h>

namespace step::video {

class Frame
{
public:
    using DataType = uint8_t;
    using DataTypePtr = DataType*;

    using Deleter = void (*)(DataTypePtr);
    static constexpr Deleter empty_deleter = [](DataTypePtr) {};
    static constexpr Deleter default_deleter = [](DataTypePtr ptr) {
        if (ptr)
        {
            delete[] ptr;
            ptr = nullptr;
        }
    };

    /*! @brief Returns a frame view over the specified data.
    */
    static Frame create(const FrameSize& size, size_t stride, PixFmt fmt, DataTypePtr data,
                        Deleter deleter = Frame::default_deleter)
    {
        Frame frame;
        frame.m_data = data;
        frame.m_deleter = deleter;
        frame.size = size;
        frame.pix_fmt = fmt;
        frame.stride = stride;

        return frame;
    }

    /*! @brief Returns a new frame that holds a copy of the data.
    */
    static Frame create_deep(const FrameSize& size, size_t stride, PixFmt fmt, DataTypePtr data)
    {
        Frame frame_view = create(size, stride, fmt, data, Frame::empty_deleter);

        // Explicit copy constructor
        Frame frame = frame_view;
        return frame;
    }

public:
    Frame() = default;

    Frame(const FrameSize& s, PixFmt fmt) : size(s), pix_fmt(fmt)
    {
        stride = calculate_stride();
        m_data = new DataType[stride * size.height];
        m_deleter = Frame::default_deleter;
    }

    Frame(const Frame& rhs) : size(rhs.size), stride(rhs.stride), pix_fmt(rhs.pix_fmt)
    {
        reset();

        m_data = new DataType[rhs.bytesize()];
        std::memcpy(m_data, rhs.m_data, bytesize());
    }
    Frame& operator=(const Frame& rhs)
    {
        Frame tmp(rhs);
        swap(*this, tmp);
        return *this;
    }

    Frame(Frame&& rhs)
        : m_data(std::exchange(rhs.m_data, nullptr))
        , m_deleter(std::exchange(rhs.m_deleter, default_deleter))
        , size(std::exchange(rhs.size, FrameSize()))
        , stride(std::exchange(rhs.stride, 0))
        , pix_fmt(std::exchange(rhs.pix_fmt, PixFmt::Undefined))
    {
    }
    Frame& operator=(Frame&& rhs)
    {
        Frame tmp(std::move(rhs));
        swap(*this, tmp);
        return *this;
    }

    ~Frame() { reset(); }

    bool operator==(const Frame& lhs) const
    {
        /* clang-format off */
        return true
            && bytesize() == lhs.bytesize()
            && size == lhs.size
            && pix_fmt == lhs.pix_fmt
            && stride == lhs.stride
            && std::memcmp(m_data, lhs.m_data, bytesize()) == 0
        ;
        /* clang-format on */
    }

    friend void swap(Frame& lhs, Frame& rhs) noexcept
    {
        std::swap(lhs.m_data, rhs.m_data);
        std::swap(lhs.m_deleter, rhs.m_deleter);
        std::swap(lhs.size, rhs.size);
        std::swap(lhs.pix_fmt, rhs.pix_fmt);
        std::swap(lhs.stride, rhs.stride);
    }

    bool is_valid() const noexcept
    {
        /* clang-format off */
        return true 
            && m_data
            && pix_fmt != PixFmt::Undefined
            && size != FrameSize()
            && stride != 0
            ;
        /* clang-format on */
    }

public:
    size_t bytesize() const noexcept { return stride * size.height; }

    size_t bpp() const { return video::utils::get_bpp(pix_fmt); }

    DataTypePtr data() noexcept { return m_data; }
    const DataType* data() const noexcept { return m_data; }

    template <typename T>
    T* data() noexcept
    {
        return static_cast<T*>(m_data);
    }

    template <typename T>
    const T* data() const noexcept
    {
        return static_cast<const T*>(m_data);
    }

private:
    size_t calculate_stride() const noexcept { return bpp() * size.width / CHAR_BIT; }

    void reset()
    {
        m_deleter(m_data);
        m_data = nullptr;
        m_deleter = default_deleter;
    }

private:
    DataTypePtr m_data{nullptr};  // pointer to data begining
    Deleter m_deleter{Frame::default_deleter};

public:
    FrameSize size;
    size_t stride{0};
    PixFmt pix_fmt{PixFmt::Undefined};
};

struct CameraFrame
{
    bool is_valid() const noexcept { return frame.is_valid(); }
    size_t bytesize() const noexcept { return frame.bytesize(); }
    size_t bpp() const noexcept { return frame.bpp(); }
    FrameSize size() const noexcept { return frame.size; }
    size_t stride() const noexcept { return frame.stride; }
    PixFmt pix_fmt() const noexcept { return frame.pix_fmt; }

    friend void swap(CameraFrame& lhs, CameraFrame& rhs) noexcept
    {
        swap(lhs.frame, rhs.frame);
        std::swap(lhs.ts, rhs.ts);
    }

    Frame frame{};
    Timestamp ts{step::get_current_timestamp()};
};

}  // namespace step::video

template <>
struct fmt::formatter<step::video::Frame> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::video::Frame& frame, FormatContext& ctx)
    {
        return format_to(ctx.out(), "size: [{}], stride: {}, bpp: {}, pix_fmt: {}, ptr: {}", frame.size, frame.stride,
                         frame.bpp(), frame.pix_fmt, (void*)frame.data());
    }
};

template <>
struct fmt::formatter<step::video::CameraFrame> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::video::CameraFrame& frame, FormatContext& ctx)
    {
        return format_to(ctx.out(), "ts: {}, {}", frame.ts, frame.frame);
    }
};
