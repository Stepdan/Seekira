#pragma once

#include "yolo_object.hpp"

#include <core/base/interfaces/serializable.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <proc/interfaces/neural_net_interface.hpp>

#include <vector>

namespace step::proc {

class YoloxWrapper
{
public:
    struct Initializer : public ISerializable
    {
        int grid_count{0};
        int class_count{0};
        float nms_threshold{0.0f};
        float prob_threshold{0.0f};
        std::vector<int> strides;
        step::video::FrameSize frame_size;

        void deserialize(const ObjectPtrJSON& container) override {}
    };

    struct GridAndStride
    {
        int grid_0;
        int grid_1;
        int stride;
    };

public:
    YoloxWrapper();

    void initialize(Initializer&& init);

    std::vector<YoloObject> process(const NeuralOutput& neural_output, const step::video::FrameSize& orig_frame_size);

private:
    void generate_grid_and_strides();
    std::vector<YoloObject> generate_yolox_proposals(const float* feat_ptr);
    std::vector<int> nms_sorted_bboxes(const std::vector<YoloObject>& objects);

private:
    int m_grid_count{0};
    int m_class_count{0};
    float m_nms_threshold{0.0f};
    float m_prob_threshold{0.0f};
    std::vector<int> m_strides;
    step::video::FrameSize m_frame_size;

    std::vector<GridAndStride> m_grid_strides;
};

}  // namespace step::proc