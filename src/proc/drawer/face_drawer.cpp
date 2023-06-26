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

    const auto match_status = face->get_match_status();
    /* clang-format off */
    const auto& color = match_status == FaceMatchStatus::Matched  ? m_settings.get_valid_face_color() :
                        match_status == FaceMatchStatus::Possible ? m_settings.get_prob_face_color()
                                                                  : m_settings.get_face_color();
    /* clang-format on */
    auto cv_color = get_cv_color(color, frame.pix_fmt);

    auto mat = video::utils::to_mat(frame);
    cv::rectangle(mat, cv::Rect(rect.p0.x, rect.p0.y, rect.length(), rect.height()), cv_color, 2);

    //cv::putText(mat, std::to_string(face->get_confidence()), cv::Point(rect.p0.x + 5, rect.p0.y + rect.length() * 0.15),
    //            cv::FONT_HERSHEY_DUPLEX, 0.5, get_cv_color(m_settings.get_face_color(), frame.pix_fmt), 1);

    // for (const auto& point : face->get_landmarks())
    //     cv::circle(mat, cv::Point2f(point.x, point.y), 1, color, 1);
}

}  // namespace step::proc