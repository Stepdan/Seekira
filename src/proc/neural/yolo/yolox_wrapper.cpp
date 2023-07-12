#include "yolox_wrapper.hpp"

#include <core/exception/assert.hpp>

namespace step::proc {

YoloxWrapper::YoloxWrapper() {}

void YoloxWrapper::initialize(Initializer&& init)
{
    /* clang-format off */
    m_grid_count        = std::move(init.grid_count);
    m_class_count       = std::move(init.class_count);
    m_nms_threshold     = std::move(init.nms_threshold);
    m_prob_threshold    = std::move(init.prob_threshold);
    m_strides           = std::move(init.strides);
    m_frame_size        = std::move(init.frame_size);
    /* clang-format on */

    generate_grid_and_strides();
}

std::vector<YoloObject> YoloxWrapper::process(const NeuralOutput& neural_output,
                                              const step::video::FrameSize& orig_frame_size)
{
    const float* data = !neural_output.data_vec.empty() ? neural_output.data_vec.data() : neural_output.data_ptr;

    auto yolo_objects = generate_yolox_proposals(data);
    std::sort(yolo_objects.begin(), yolo_objects.end(),
              [](const auto& item0, const auto& item1) { return item0.prob > item1.prob; });

    auto nms_sorted_indexes = nms_sorted_bboxes(yolo_objects);

    double width_scale = 1.0 * orig_frame_size.width / m_frame_size.width;
    double height_scale = 1.0 * orig_frame_size.height / m_frame_size.height;

    std::vector<YoloObject> result;
    result.reserve(nms_sorted_indexes.size());
    std::transform(nms_sorted_indexes.cbegin(), nms_sorted_indexes.cend(), std::back_inserter(result),
                   [this, &yolo_objects, &width_scale, &height_scale](int nms_sorted_index) {
                       auto obj = yolo_objects[nms_sorted_index];

                       // Recalculate bboxes to orig frame size
                       const auto x0 = obj.rect.x * width_scale;
                       const auto x1 = (obj.rect.x + obj.rect.width) * width_scale;
                       const auto y0 = obj.rect.y * height_scale;
                       const auto y1 = (obj.rect.y + obj.rect.height) * height_scale;

                       obj.rect.x = x0;
                       obj.rect.width = x1 - x0;
                       obj.rect.y = y0;
                       obj.rect.height = y1 - y0;

                       return obj;
                   });

    return result;
}

void YoloxWrapper::generate_grid_and_strides()
{
    m_grid_strides.clear();
    for (int i = 0; i < m_strides.size(); ++i)
    {
        int stride = m_strides[i];
        int num_grid_w = m_frame_size.width / stride;
        int num_grid_h = m_frame_size.height / stride;
        for (int g1 = 0; g1 < num_grid_h; ++g1)
        {
            for (int g0 = 0; g0 < num_grid_w; ++g0)
            {
                GridAndStride gs;
                gs.grid_0 = g0;
                gs.grid_1 = g1;
                gs.stride = stride;
                m_grid_strides.push_back(gs);
            }
        }
    }
}

std::vector<YoloObject> YoloxWrapper::generate_yolox_proposals(const float* feat_ptr)
{
    std::vector<YoloObject> yolo_objects;

    try
    {
        const int num_anchors = m_grid_strides.size();
        for (int anchor_idx = 0; anchor_idx < num_anchors; anchor_idx++)
        {
            const int grid0 = m_grid_strides[anchor_idx].grid_0;
            const int grid1 = m_grid_strides[anchor_idx].grid_1;
            const int stride = m_grid_strides[anchor_idx].stride;

            // yolox/models/yolo_head.py decode logic
            //  outputs[..., :2] = (outputs[..., :2] + grids) * strides
            //  outputs[..., 2:4] = torch.exp(outputs[..., 2:4]) * strides
            float x_center = (feat_ptr[0] + grid0) * stride;
            float y_center = (feat_ptr[1] + grid1) * stride;
            float w = exp(feat_ptr[2]) * stride;
            float h = exp(feat_ptr[3]) * stride;
            float x0 = x_center - w * 0.5f;
            float y0 = y_center - h * 0.5f;

            float box_objectness = feat_ptr[4];
            for (int class_idx = 0; class_idx < m_class_count; ++class_idx)
            {
                float box_cls_score = feat_ptr[5 + class_idx];
                float box_prob = box_objectness * box_cls_score;
                if (box_prob > m_prob_threshold)
                {
                    YoloObject obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = w;
                    obj.rect.height = h;
                    obj.label = class_idx;
                    obj.prob = box_prob;

                    yolo_objects.push_back(obj);
                }

            }  // class loop

            // TODO точно ли надо 5
            feat_ptr += (m_class_count + 5);

        }  // point anchor loop
    }
    catch (std::exception& e)
    {
        STEP_LOG(L_ERROR, "Exception handled due generate_yolox_proposals: {}", e.what());
        return {};
    }
    catch (...)
    {
        STEP_LOG(L_ERROR, "Unknown exception handled due generate_yolox_proposals");
        return {};
    }

    return yolo_objects;
}

std::vector<int> YoloxWrapper::nms_sorted_bboxes(const std::vector<YoloObject>& objects)
{
    std::vector<int> result;

    std::vector<float> areas;
    areas.reserve(objects.size());
    std::transform(objects.cbegin(), objects.cend(), std::back_inserter(areas),
                   [](const auto& item) { return item.rect.area(); });

    for (int i = 0; i < objects.size(); ++i)
    {
        const auto& obj0 = objects[i];

        bool need_keep = true;
        for (int j = 0; j < result.size(); ++j)
        {
            const auto& obj1 = objects[result[j]];

            // intersection over union
            float inter_area = (obj0.rect & obj1.rect).area();
            float union_area = areas[i] + areas[result[j]] - inter_area;
            // float IoU = inter_area / union_area
            if (inter_area / union_area > m_nms_threshold)
            {
                need_keep = false;
                break;
            }
        }

        if (need_keep)
            result.push_back(i);
    }

    return result;
}

}  // namespace step::proc