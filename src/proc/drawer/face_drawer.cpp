#include "drawer.hpp"

#include <core/exception/assert.hpp>

#include <proc/interfaces/face.hpp>

#include <video/frame/utils/frame_utils_opencv.hpp>

#include <opencv2/imgproc.hpp>

namespace step::proc {

template <>
void Drawer::draw(video::Frame& frame, const FacePtr& face)
{
    STEP_ASSERT(face, "Invalid face to draw!");

    const auto rect = face->get_rect();

    auto mat = video::utils::to_mat(frame);
    cv::rectangle(mat, cv::Rect(rect.p0.x, rect.p0.y, std::abs(rect.p1.x - rect.p0.x), std::abs(rect.p1.y - rect.p0.y)),
                  get_cv_color(m_settings.get_face_color(), frame.pix_fmt), 2);
}

}  // namespace step::proc