#include <video/frame/utils/frame_utils.hpp>

#include <core/log/log.hpp>

#include <proc/detect/registrator.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <set>

using namespace step;
using namespace step::video;
using namespace step::video::utils;

struct TestDataProvider
{
    static std::filesystem::path test_data_dir()
    {
#ifndef FACE_DETECTOR_TESTS_DATA_DIR
#error "FACE_DETECTOR_TESTS_DATA_DIR must be defined and point to valid testdata folder"
#endif
        std::filesystem::path path(FACE_DETECTOR_TESTS_DATA_DIR);
        assert(std::filesystem::is_directory(path));
        return path;
    }

    // hardcoded image.png data for tests usage
    static std::string frame_path() { return test_data_dir().append("image.jpg").string(); }
    static std::string saved_frame_path() { return test_data_dir().append("saved_image.png").string(); }
};

class FaceDetectorTest : public ::testing::Test
{
protected:
    void SetUp()
    {
        frame = video::utils::open_file(TestDataProvider::frame_path());
        auto settings_ptr = std::make_shared<proc::SettingsFaceDetector>();
        settings_ptr->set_face_engine_type(proc::FaceEngineType::TDV);
        settings_ptr->set_mode(proc::IFaceEngine::Mode::FE_DETECTION);
        settings_ptr->set_model_path("C:/Work/StepTech/SDK/models/");
        m_detector = proc::create_face_detector(settings_ptr);
    }

    Frame frame;
    std::unique_ptr<step::proc::IDetector> m_detector;
};

TEST_F(FaceDetectorTest, face_detection_test)
{
    proc::DetectionResult result;
    EXPECT_NO_THROW(result = m_detector->process(frame));
    const auto& bboxes = result.bboxes();
    ASSERT_EQ(bboxes.size(), 3);
    for (const auto& bbox : bboxes)
    {
        STEP_LOG(L_INFO, "Face bbox: {}", bbox);
    }
}