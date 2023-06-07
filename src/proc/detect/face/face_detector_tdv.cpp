#include "face_detector_factory.hpp"

#include <core/log/log.hpp>

#include <video/frame/utils/frame_utils.hpp>
#include <video/frame/utils/frame_utils_opencv.hpp>

#include <thirdparty/tdv/api/Service.h>

namespace {

static const std::map<int, std::string> CvTypeToStr{{CV_8U, "uint8_t"},  {CV_8S, "int8_t"},   {CV_16U, "uint16_t"},
                                                    {CV_16S, "int16_t"}, {CV_32S, "int32_t"}, {CV_32F, "float"},
                                                    {CV_64F, "double"}};

void cvMatToBSM(api::Context& bsmCtx, const cv::Mat& img, bool copy = false)
{
    const cv::Mat& input_img = img.isContinuous() ? img : img.clone();  // setDataPtr requires continuous data
    size_t copy_sz = (copy || !img.isContinuous()) ? input_img.total() * input_img.elemSize() : 0;
    bsmCtx["format"] = "NDARRAY";
    bsmCtx["blob"].setDataPtr(input_img.data, copy_sz);
    bsmCtx["dtype"] = CvTypeToStr.at(input_img.depth());
    for (int i = 0; i < input_img.dims; ++i)
        bsmCtx["shape"].push_back(input_img.size[i]);
    bsmCtx["shape"].push_back(input_img.channels());
}

}  // namespace

namespace step::proc {

class FaceDetectorTDV : public BaseDetector<SettingsFaceDetector>
{
public:
    FaceDetectorTDV(const std::shared_ptr<task::BaseSettings>& settings)
    {
        set_settings(*settings);
        const auto sdk_path = m_typed_settings.get_model_path();

        m_service = std::make_unique<api::Service>(api::Service::createService(sdk_path));
        m_context = std::make_unique<api::Context>(m_service->createContext());
        (*m_context)["unit_type"] = "FACE_DETECTOR";

        m_detector = std::make_unique<api::ProcessingBlock>(m_service->createProcessingBlock(*m_context));
    }

    DetectionResult process(video::Frame& frame) override
    {
        STEP_LOG(L_INFO, "FaceDetectorTDV process");
        auto copied = frame;
        video::utils::convert_colorspace(copied, video::PixFmt::RGB);
        auto mat = video::utils::to_mat(copied);

        auto ioData = m_service->createContext();
        auto imgCtx = m_service->createContext();

        cvMatToBSM(imgCtx, mat);
        ioData["image"] = imgCtx;
        (*m_detector)(ioData);

        std::vector<DetectionBBox> bboxes;

        for (const api::Context& obj : ioData["objects"])
        {
            if (obj["class"].getString().compare("face"))
                continue;

            const api::Context& rectCtx = obj.at("bbox");
            bboxes.push_back({static_cast<int>(rectCtx[0].getDouble() * mat.cols),
                              static_cast<int>(rectCtx[1].getDouble() * mat.rows),
                              static_cast<int>(rectCtx[2].getDouble() * mat.cols),
                              static_cast<int>(rectCtx[3].getDouble() * mat.rows)});
        }

        return DetectionResult(std::move(bboxes));
    }

private:
    std::unique_ptr<api::Service> m_service;
    std::unique_ptr<api::Context> m_context;
    std::unique_ptr<api::ProcessingBlock> m_detector;
};

std::unique_ptr<IDetector> create_face_detector_tdv(const std::shared_ptr<task::BaseSettings>& settings)
{
    return std::make_unique<FaceDetectorTDV>(settings);
}

}  // namespace step::proc