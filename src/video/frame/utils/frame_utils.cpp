#include "frame_utils.hpp"
#include "frame_utils_opencv.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace step::video::utils {

void convert_colorspace(Frame& frame, PixFmt dst_format)
{
    if (dst_format == frame.pix_fmt)
        return;

    cv::Mat src = to_mat(frame);
    cv::Mat dst;
    cv::cvtColor(src, dst, utils::get_colorspace_convert_id(frame.pix_fmt, dst_format));
    frame = Frame::create_deep({static_cast<size_t>(dst.cols), static_cast<size_t>(dst.rows)}, dst.step, dst_format,
                               dst.data);
}

void covert_or_clone(Frame& frame, PixFmt dst_format)
{
    // clone if format is the same
    if (dst_format == frame.pix_fmt)
    {
        // dummy exchange to ensure we've cloned the memory
        Frame clone(frame);
        frame = std::move(clone);
        return;
    }
    // convert the color space otherwise
    convert_colorspace(frame, dst_format);
}

}  // namespace step::video::utils
