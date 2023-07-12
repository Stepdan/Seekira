#include "drawer.hpp"

#include <core/exception/assert.hpp>

#include <core/base/types/rect.hpp>

#include <video/frame/utils/frame_utils_opencv.hpp>

#include <opencv2/imgproc.hpp>

namespace step::proc {

template <>
void Drawer::draw(video::Frame& frame, const std::vector<Rect>& bboxes)
{
    auto cv_color = get_cv_color(m_settings.get_face_color(), frame.pix_fmt);

    auto mat = video::utils::to_mat(frame);
    for (const auto& rect : bboxes)
        cv::rectangle(mat, cv::Rect(rect.p0.x, rect.p0.y, rect.length(), rect.height()), cv_color, 2);
}

}  // namespace step::proc