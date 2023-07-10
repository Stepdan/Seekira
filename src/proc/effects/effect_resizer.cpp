#include "registrator.hpp"

#include <core/log/log.hpp>

#include <video/frame/utils/frame_utils_opencv.hpp>

#include <proc/settings/settings_resizer.hpp>

#include <opencv2/opencv.hpp>

namespace step::proc {

class EffectResizer : public BaseEffect<SettingsResizer>
{
public:
    EffectResizer(const std::shared_ptr<task::BaseSettings>& settings) { set_settings(*settings); }

    video::Frame process(video::Frame& input) override
    {
        auto new_frame_size = m_typed_settings.get_frame_size();
        if (new_frame_size == input.size)
            return input;

        auto frame = video::Frame::clone_deep(input);
        auto frame_mat = video::utils::to_mat(frame);

        const auto size_mode = m_typed_settings.get_size_mode();
        switch (size_mode)
        {
            case SettingsResizer::SizeMode::HalfDownscale:
                new_frame_size.width /= 2;
                new_frame_size.height /= 2;
                break;

            case SettingsResizer::SizeMode::QuaterDownscale:
                new_frame_size.width /= 4;
                new_frame_size.height /= 4;
                break;

            case SettingsResizer::SizeMode::ScaleByMax:
                if (new_frame_size.width > new_frame_size.height)
                    new_frame_size.height = 1.0 * frame.size.height * new_frame_size.width / frame.size.width;
                else
                    new_frame_size.width = 1.0 * frame.size.width * new_frame_size.height / frame.size.height;
                break;

            case SettingsResizer::SizeMode::Padding:
                [[fallthrough]];
            case SettingsResizer::SizeMode::ScaleByMin:
                if (new_frame_size.width < new_frame_size.height)
                    new_frame_size.height = 1.0 * frame.size.height * new_frame_size.width / frame.size.width;
                else
                    new_frame_size.width = 1.0 * frame.size.width * new_frame_size.height / frame.size.height;
                break;
        }

        STEP_LOG(L_DEBUG, "Try to resize frame from {} to {}", frame.size, new_frame_size);

        cv::resize(frame_mat, frame_mat, video::utils::get_cv_size(new_frame_size), 0.0, 0.0,
                   video::utils::get_cv_interpolation(m_typed_settings.get_interpolation()));

        if (size_mode == SettingsResizer::SizeMode::Padding)
        {
            const auto pad_size = m_typed_settings.get_frame_size();
            STEP_LOG(L_DEBUG, "Try to pad frame from {} to {}", new_frame_size, pad_size);
            cv::Mat pad_frame = cv::Mat(video::utils::get_cv_size(pad_size), frame_mat.type(), cv::Scalar());
            frame_mat.copyTo(pad_frame(cv::Rect(0, 0, new_frame_size.width, new_frame_size.height)));
            frame_mat = pad_frame.clone();
        }

        return video::utils::from_mat_deep(frame_mat, frame.pix_fmt);
    }

private:
};

std::unique_ptr<IEffect> create_effect_resizer(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<EffectResizer>(settings);
}

}  // namespace step::proc