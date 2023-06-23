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

    video::Frame process(EffectMultiInput& input) override
    {
        STEP_ASSERT(input.size() == 1, "Invalid EffectMultiInput size in resizer!");
        video::FrameSize new_frame_size = m_typed_settings.get_frame_size();
        if (new_frame_size == input.front().size)
            return input.front();

        auto frame = video::Frame::clone_deep(input.front());
        auto frame_mat = video::utils::to_mat(frame);

        const auto size_mode = m_typed_settings.get_size_mode();
        switch (m_typed_settings.get_size_mode())
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

            case SettingsResizer::SizeMode::ScaleByMin:
                if (new_frame_size.width < new_frame_size.height)
                    new_frame_size.height = 1.0 * frame.size.height * new_frame_size.width / frame.size.width;
                else
                    new_frame_size.width = 1.0 * frame.size.width * new_frame_size.height / frame.size.height;
                break;
        }

        STEP_LOG(L_DEBUG, "Try resize frame from {} to {}", frame.size, new_frame_size);

        cv::resize(frame_mat, frame_mat, video::utils::get_cv_size(new_frame_size), 0.0, 0.0,
                   video::utils::get_cv_interpolation(m_typed_settings.get_interpolation()));

        return video::utils::from_mat_deep(frame_mat, frame.pix_fmt);
    }
};

std::unique_ptr<IEffect> create_effect_resizer(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<EffectResizer>(settings);
}

}  // namespace step::proc