#include "face_engine_tdv.hpp"
#include "face_tdv.hpp"

#include <core/log/log.hpp>
#include <core/exception/assert.hpp>

#include <video/frame/utils/frame_utils.hpp>
#include <video/frame/utils/frame_utils_opencv.hpp>

#include <thirdparty/tdv/api/Service.h>
#include <thirdparty/tdv/data/Context.h>

#include <opencv2/opencv.hpp>

namespace {

/* clang-format off */
const std::string UNIT_TYPE = "unit_type";
const std::string USE_CUDA = "use_cuda";

const std::string FACE_DETECTOR_UNIT_NAME      = "FACE_DETECTOR";
const std::string FACE_RECOGNIZER_UNIT_NAME    = "FACE_RECOGNIZER";
const std::string FACE_LANDMARKS_UNIT_NAME     = "MESH_FITTER";
const std::string FACE_MATCHER_UNIT_NAME       = "MATCHER_MODULE";
/* clang-format on */

/* clang-format off */
const std::map<int, std::string> g_cvtype_to_str {
    { CV_8U , "uint8_t"  },
    { CV_8S , "int8_t"   },
    { CV_16U, "uint16_t" },
    { CV_16S, "int16_t"  },
    { CV_32S, "int32_t"  },
    { CV_32F, "float"    },
    { CV_64F, "double"   }
};
/* clang-format on */

void mat_to_bsm(api::Context& bsm_ctx, const cv::Mat& img, bool copy = false)
{
    const cv::Mat& input_img = img.isContinuous() ? img : img.clone();  // setDataPtr requires continuous data
    size_t copy_sz = (copy || !img.isContinuous()) ? input_img.total() * input_img.elemSize() : 0;
    bsm_ctx["format"] = "NDARRAY";
    bsm_ctx["blob"].setDataPtr(input_img.data, copy_sz);
    bsm_ctx["dtype"] = g_cvtype_to_str.at(input_img.depth());
    for (int i = 0; i < input_img.dims; ++i)
        bsm_ctx["shape"].push_back(input_img.size[i]);
    bsm_ctx["shape"].push_back(input_img.channels());
}

}  // namespace

namespace step::proc {

class FaceEngineTDV : public BaseFaceEngine
{
public:
    FaceEngineTDV(IFaceEngine::Initializer&& init) : BaseFaceEngine(std::move(init)) { load_models(); }

    Faces detect(const video::Frame& frame) override
    {
        STEP_ASSERT(m_mode & FE_DETECTION, "Can't detect faces: wrong mode!");
        STEP_ASSERT(m_detector_ctx, "Can't detect faces: invalid context!");
        STEP_ASSERT(m_face_detector, "Can't detect faces: invalid proc block!");

        auto copied = frame;
        video::utils::convert_colorspace(copied, video::PixFmt::RGB);
        auto mat = video::utils::to_mat(copied);

        cv::resize(mat, mat, cv::Size(copied.size.width, copied.size.height));

        auto io_data = m_service->createContext();
        auto img_ctx = m_service->createContext();

        mat_to_bsm(img_ctx, mat);
        io_data["image"] = img_ctx;
        (*m_face_detector)(io_data);

        Faces faces;

        for (const api::Context& obj : io_data["objects"])
        {
            if (obj["class"].getString().compare("face"))
                continue;

            auto face = FaceTDV::create_face();
            const api::Context& rect_ctx = obj.at("bbox");
            face->set_rect(Rect({static_cast<int>(rect_ctx[0].getDouble() * frame.size.width),
                                 static_cast<int>(rect_ctx[1].getDouble() * frame.size.height),
                                 static_cast<int>(rect_ctx[2].getDouble() * frame.size.width),
                                 static_cast<int>(rect_ctx[3].getDouble() * frame.size.height)}));

            face->set_confidence(obj.at("confidence").getDouble());

            face->set_impl_data(std::make_shared<api::Context>(obj));

            /*
            TODO Crop face
            if (m_save_frames)
                face->set_frame(video::utils::crop_frame_deep(copied, face->get_rect()));
            */

            if (m_mode & FE_LANDMARKS)
                calc_landmarks(copied, face);

            faces.push_back(face);
        }

        // TODO check faces for duplicating, etc...

        return faces;
    }

    void recognize(const FacePtr& face) override
    {
        STEP_ASSERT(m_mode & FE_RECOGNITION, "Can't recognize faces: wrong mode!");
        STEP_ASSERT(m_recognizer_ctx, "Can't recognize faces: invalid context!");
        STEP_ASSERT(m_recognizer_module, "Can't recognize faces: invalid proc block!");

        auto face_tdv = std::dynamic_pointer_cast<FaceTDV>(face);
        STEP_ASSERT(face_tdv, "Invalid FaceTDV cast!");

        auto impl_data = face_tdv->get_impl_data();
        STEP_ASSERT(impl_data, "Invalid FaceTDV impl data!");

        (*m_recognizer_module)(*impl_data);

        auto recognizer_data_ctx = (*impl_data)["template"];
        //auto recognizer_data_size_ctx = (*impl_data)["template_size"];
        if (recognizer_data_ctx.isNone())  // || recognizer_data_size_ctx.isNone())
        {
            STEP_LOG(L_ERROR, "Can't recognize face!");
            return;
        }

        try
        {
            //auto data_size = recognizer_data_size_ctx.getLong();
            face->set_recognizer_data(
                reinterpret_cast<tdv::data::Context*>(recognizer_data_ctx.getHandle())->get<std::vector<float>>());
        }
        catch (const std::exception& e)
        {
            STEP_LOG(L_ERROR, "Exception handled due face recognize: {}", e.what());
            return;
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Unknown exception handled due face recognize");
            return;
        }
    }

    bool compare(const FacePtr& face0, const FacePtr& face1) override
    {
        STEP_ASSERT(m_mode & FE_RECOGNITION, "Can't compare faces: wrong mode!");
        STEP_ASSERT(m_matcher_ctx, "Can't compare faces: invalid context!");
        STEP_ASSERT(m_matcher_module, "Can't compare faces: invalid proc block!");

        auto face0_tdv = std::dynamic_pointer_cast<FaceTDV>(face0);
        auto face1_tdv = std::dynamic_pointer_cast<FaceTDV>(face1);
        STEP_ASSERT(face0_tdv && face1_tdv, "Invalid FaceTDV cast!");

        auto impl0_data = face0_tdv->get_impl_data();
        auto impl1_data = face0_tdv->get_impl_data();
        STEP_ASSERT(impl0_data && impl1_data, "Invalid FaceTDV impl data!");

        api::Context matcher_data = m_service->createContext();
        matcher_data["verification"]["objects"].push_back(*impl0_data);
        matcher_data["verification"]["objects"].push_back(*impl1_data);
        (*m_matcher_module)(matcher_data);

        return matcher_data["verification"]["result"]["verdict"].getBool();
    }

protected:
    void calc_landmarks(const video::Frame& frame, const FacePtr& face) override
    {
        STEP_ASSERT(m_mode & FE_LANDMARKS, "Can't find landmarks: wrong mode!");
        STEP_ASSERT(m_fitter_ctx, "Can't find landmarks: invalid context!");
        STEP_ASSERT(m_mesh_fitter, "Can't find landmarks: invalid proc block!");

        auto face_tdv = std::dynamic_pointer_cast<FaceTDV>(face);
        STEP_ASSERT(face_tdv, "Invalid FaceTDV cast!");

        auto impl_data = face_tdv->get_impl_data();
        STEP_ASSERT(impl_data, "Invalid FaceTDV impl data!");

        (*m_mesh_fitter)(*impl_data);

        auto fitter = (*impl_data)["fitter"];
        if (fitter.isNone())
        {
            STEP_LOG(L_ERROR, "Can't calc landmarks!");
            return;
        }

        FaceLandmarks landmarks;
        for (const api::Context& point : fitter["keypoints"])
            landmarks.push_back({static_cast<int>(point[0].getDouble() * frame.size.width),
                                 static_cast<int>(point[1].getDouble() * frame.size.height)});
        landmarks.shrink_to_fit();

        face->set_landmarks(landmarks);
    }

private:
    bool load_models() override
    {
        if (m_models_path.empty() || !std::filesystem::is_directory(m_models_path) ||
            m_models_path.string().back() != '/')
        {
            STEP_LOG(L_ERROR, "Can't load models for FaceEngineTDV, invalid models dir path: {}",
                     m_models_path.string());
            return false;
        }

        try
        {
            m_service = std::make_unique<api::Service>(api::Service::createService(m_models_path.string()));

            const auto use_cuda = m_device_type == DeviceType::GPU;

            if (m_mode & FE_DETECTION)
            {
                m_detector_ctx = std::make_unique<api::Context>(m_service->createContext());
                (*m_detector_ctx)[UNIT_TYPE] = FACE_DETECTOR_UNIT_NAME;
                (*m_detector_ctx)[USE_CUDA] = use_cuda;
                m_face_detector =
                    std::make_unique<api::ProcessingBlock>(m_service->createProcessingBlock(*m_detector_ctx));
            }

            if (m_mode & FE_LANDMARKS)
            {
                m_fitter_ctx = std::make_unique<api::Context>(m_service->createContext());
                (*m_fitter_ctx)[UNIT_TYPE] = FACE_LANDMARKS_UNIT_NAME;
                (*m_fitter_ctx)[USE_CUDA] = use_cuda;
                m_mesh_fitter = std::make_unique<api::ProcessingBlock>(m_service->createProcessingBlock(*m_fitter_ctx));
            }

            if (m_mode & FE_RECOGNITION)
            {
                m_recognizer_ctx = std::make_unique<api::Context>(m_service->createContext());
                (*m_recognizer_ctx)[UNIT_TYPE] = FACE_RECOGNIZER_UNIT_NAME;
                (*m_recognizer_ctx)[USE_CUDA] = use_cuda;
                m_recognizer_module =
                    std::make_unique<api::ProcessingBlock>(m_service->createProcessingBlock(*m_recognizer_ctx));

                m_matcher_ctx = std::make_unique<api::Context>(m_service->createContext());
                (*m_matcher_ctx)[UNIT_TYPE] = FACE_MATCHER_UNIT_NAME;
                m_matcher_module =
                    std::make_unique<api::ProcessingBlock>(m_service->createProcessingBlock(*m_matcher_ctx));
            }
        }
        catch (std::exception& e)
        {
            STEP_LOG(L_ERROR, "Exception handled due FaceEngineTDV::load_models: {}", e.what());
            reset();
            return false;
        }
        catch (...)
        {
            STEP_LOG(L_ERROR, "Unknown exception handled due FaceEngineTDV::load_models");
            reset();
            return false;
        }

        return true;
    }

    void reset()
    {
        m_detector_ctx.reset();
        m_face_detector.reset();

        m_fitter_ctx.reset();
        m_mesh_fitter.reset();

        m_recognizer_ctx.reset();
        m_recognizer_module.reset();

        m_matcher_ctx.reset();
        m_matcher_module.reset();

        m_service.reset();
    }

private:
    std::unique_ptr<api::Service> m_service;

    std::unique_ptr<api::Context> m_detector_ctx;
    std::unique_ptr<api::Context> m_fitter_ctx;
    std::unique_ptr<api::Context> m_recognizer_ctx;
    std::unique_ptr<api::Context> m_matcher_ctx;

    std::unique_ptr<api::ProcessingBlock> m_face_detector;
    std::unique_ptr<api::ProcessingBlock> m_mesh_fitter;
    std::unique_ptr<api::ProcessingBlock> m_recognizer_module;
    std::unique_ptr<api::ProcessingBlock> m_matcher_module;
};

}  // namespace step::proc

namespace step::proc {

std::shared_ptr<IFaceEngine> create_face_engine_tdv(IFaceEngine::Initializer&& init)
{
    return std::make_shared<FaceEngineTDV>(std::move(init));
}

}  // namespace step::proc