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
    cv::rectangle(mat, cv::Rect(rect.p0.x, rect.p0.y, rect.length(), rect.height()),
                  get_cv_color(m_settings.get_face_color(), frame.pix_fmt), 2);

    cv::putText(mat, std::to_string(face->get_confidence()), cv::Point(rect.p0.x + 5, rect.p0.y + rect.length() * 0.15),
                cv::FONT_HERSHEY_DUPLEX, 0.5, get_cv_color(m_settings.get_face_color(), frame.pix_fmt), 1);
}

}  // namespace step::proc