#include "registrator.hpp"

#include <core/log/log.hpp>
#include <core/base/utils/type_utils.hpp>
#include <core/base/utils/time_utils.hpp>

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
            switch (m_typed_settings.get_device_type())
            {
                case DeviceType::CPU:
                    break;

                case DeviceType::CUDA: {
                    OrtCUDAProviderOptions options;
                    options.device_id = 0;  // TODO передавать конкретный device_id
                    options.arena_extend_strategy = 0;
                    options.gpu_mem_limit = SIZE_MAX;
                    options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearch::OrtCudnnConvAlgoSearchExhaustive;
                    options.do_copy_in_default_stream = 1;
                    m_ort_session_options->AppendExecutionProvider_CUDA(options);
                }
                break;

                default:
                    STEP_UNDEFINED("Undefined device type for onnxruntime");
            }

            m_ort_session =
                std::make_unique<Ort::Session>(*m_ort_env.get(), model_path.c_str(), *m_ort_session_options.get());

            m_input_count = m_ort_session->GetInputCount();
            m_ouput_count = m_ort_session->GetOutputCount();

            Ort::AllocatorWithDefaultOptions allocator;
            {
                auto input_type_info = m_ort_session->GetInputTypeInfo(0);
                auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
                m_input_shape = input_tensor_info.GetShape();
                m_input_size = step::video::FrameSize(m_input_shape[3], m_input_shape[2]);

                for (int i = 0; i < m_input_count; ++i)
                    m_input_names.push_back(m_ort_session->GetInputName(i, allocator));

                for (int i = 0; i < m_ouput_count; ++i)
                    m_output_names.push_back(m_ort_session->GetOutputName(i, allocator));
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

        if (norm_values.size() == mat.channels() && norm_values.size() > 1)
        {
            std::vector<cv::Mat> channels;
            cv::split(mat, channels);
            for (int i = 0; i < norm_values.size(); ++i)
                channels[i] *= 1 / norm_values[i];
            cv::merge(channels, mat);
        }

        cv::Scalar mean(0, 0, 0);
        if (mean_values.size() == mat.channels() && mean_values.size() > 1)
        {
            mean = cv::Scalar(mean_values[0] / norm_values[0], mean_values[1] / norm_values[1],
                              mean_values[2] / norm_values[2]);
        }

        cv::Mat blob =
            cv::dnn::blobFromImage(mat, 1 / 255.0, cv::Size(frame.size.width, frame.size.height), mean, swap_rb, false);

        switch (m_typed_settings.get_device_type())
        {
            case DeviceType::CPU:
                return process_cpu(blob);
            case DeviceType::CUDA:
                return process_cpu(blob);

            default:
                STEP_UNDEFINED("Undefined device type for onnxruntime");
        }

        return {};
    }

private:
    NeuralOutput process_cpu(cv::Mat& blob)
    {
        auto allocator_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

        // create input tensor object from data values
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, reinterpret_cast<float*>(blob.data), blob.total(), m_input_shape.data(), m_input_shape.size());
        STEP_ASSERT(input_tensor.IsTensor(), "Invalid OnnxRuntime tensor");

        {
            utils::ExecutionTimer<Milliseconds> timer("ORT");

            auto ort_outputs = m_ort_session->Run(Ort::RunOptions(nullptr), &m_input_names[0], &input_tensor, 1,
                                                  m_output_names.data(), m_output_names.size());
            const float* out = ort_outputs[0].GetTensorMutableData<float>();
        }

        return {};
    }

    NeuralOutput process_cuda(cv::Mat& blob) { return {}; }

private:
    size_t m_input_count{0};
    size_t m_ouput_count{0};
    video::FrameSize m_input_size;

    std::vector<char*> m_input_names;
    std::vector<char*> m_output_names;
    std::vector<int64_t> m_input_shape;

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