#include "drawer.hpp"

#include <core/exception/assert.hpp>

namespace step::proc {

cv::Scalar Drawer::get_cv_color(const ColorRGB& color, video::PixFmt fmt)
{
    /* clang-format off */
    const auto is_rgb_family = false
                         || fmt == video::PixFmt::RGB
                         || fmt == video::PixFmt::RGBA
    ;

    const auto is_bgr_family = false
                         || fmt == video::PixFmt::BGR
                         || fmt == video::PixFmt::BGRA
    ;
    /* clang-format on */
    STEP_ASSERT(is_rgb_family || is_bgr_family, "Can't convert color to cv scalar: fmt is not rgb family");

    if (is_rgb_family)
        return cv::Scalar(color.red, color.green, color.blue);

    if (is_bgr_family)
        return cv::Scalar(color.blue, color.green, color.red);
}

}  // namespace step::proc