#include "registrator.hpp"

#include <core/log/log.hpp>
#include <core/base/utils/type_utils.hpp>

#include <video/frame/utils/frame_utils_opencv.hpp>

#include <proc/settings/settings_neural_onnxruntime.hpp>

#include <onnxruntime_cxx_api.h>

#include <opencv2/dnn.hpp>

namespace step::proc {

class OnnxRuntimeNeuralNet : public BaseNeuralNet<SettingsNeuralOnnxRuntime>
{
public:
    OnnxRuntimeNeuralNet(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);
        const auto& model_path = m_typed_settings.get_model_path();
        try
        {
            m_ort_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, STEPKIT_MODULE_NAME);

            m_ort_session_options = std::make_unique<Ort::SessionOptions>();
            m_ort_session_options->SetGraphOptimizationLevel(ORT_ENABLE_ALL);

            m_ort_session =
                std::make_unique<Ort::Session>(*m_ort_env.get(), model_path.c_str(), *m_ort_session_options.get());

            m_input_count = m_ort_session->GetInputCount();
            m_ouput_count = m_ort_session->GetOutputCount();

            {
                auto input_type_info = m_ort_session->GetInputTypeInfo(0);
                auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
                auto input_dims = input_tensor_info.GetShape();
                m_input_size = step::video::FrameSize(input_dims[3], input_dims[2]);
            }

            STEP_LOG(L_INFO, "Loaded base onnxruntime net: inputs: {}, outputs: {}, input size: {}, path: {}",
                     m_input_count, m_ouput_count, m_input_size, model_path.string());
        }
        catch (std::exception& ex)
        {
            STEP_LOG(L_ERROR, "Catch exception due OnnxRuntime read net: {}", ex.what());
            throw;
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Catch unknown exception due OnnxRuntime read net");
            throw;
        }
    }

    NeuralOutput process(video::Frame& frame)
    {
        auto mat = video::utils::to_mat_deep(frame);
        bool swap_rb = true;
        const auto& mean_values = m_typed_settings.get_means();
        const auto& norm_values = m_typed_settings.get_norms();

        std::vector<cv::Mat> channels;
        cv::split(mat, channels);
        for (int i = 0; i < norm_values.size(); ++i)
            channels[i] *= 1 / norm_values[i];
        cv::merge(channels, mat);

        cv::Scalar mean = mean_values.size() != 3
                              ? cv::Scalar(0, 0, 0)
                              : cv::Scalar(mean_values[0] / norm_values[0], mean_values[1] / norm_values[1],
                                           mean_values[2] / norm_values[2]);

        cv::Mat blob =
            cv::dnn::blobFromImage(mat, 1 / 255.0, cv::Size(frame.size.width, frame.size.height), mean, swap_rb, false);

        // array<int64_t, 4> input_shape_{1, 3, row, col};
        // auto allocator_info = MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        // Value input_tensor_ = Value::CreateTensor<float>(allocator_info, input_image_.data(), input_image_.size(),
        //                                                  input_shape_.data(), input_shape_.size());
        // vector<Value> ort_outputs = session_->Run(RunOptions{nullptr}, &input_names[0], &input_tensor_, 1,
        //                                           output_names.data(), output_names.size());

        return {};
    }

private:
    size_t m_input_count{0};
    size_t m_ouput_count{0};
    video::FrameSize m_input_size;

    std::unique_ptr<Ort::Session> m_ort_session;
    std::unique_ptr<Ort::Env> m_ort_env;
    std::unique_ptr<Ort::SessionOptions> m_ort_session_options;
};

std::unique_ptr<INeuralNet> create_onnxruntime_neural_net(const std::shared_ptr<task::BaseSettings>& settings)
{
    STEP_ASSERT(settings, "Can't create person detector: empty settings!");

    return std::make_unique<OnnxRuntimeNeuralNet>(settings);
}

}  // namespace step::proc