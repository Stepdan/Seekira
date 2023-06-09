#pragma once

#include <core/base/types/rect.hpp>
#include <core/base/types/meta_storage.hpp>

#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

namespace step::proc {

class DetectionResult
{
public:
    DetectionResult() = default;
    DetectionResult(std::vector<Rect>&& bboxes) : m_bboxes(std::move(bboxes)) {}
    DetectionResult(std::vector<Rect>&& bboxes, MetaStorage&& data)
        : m_bboxes(std::move(bboxes)), m_data(std::move(data))
    {
    }

    const std::vector<Rect>& bboxes() const noexcept { return m_bboxes; }
    const MetaStorage& data() const noexcept { return m_data; }

private:
    std::vector<Rect> m_bboxes;
    MetaStorage m_data;
};

using IDetector = task::ITask<video::Frame&, DetectionResult>;
using DetectorPtr = std::unique_ptr<IDetector>;

template <typename TSettings>
using BaseDetector = task::BaseTask<TSettings, video::Frame&, DetectionResult>;

}  // namespace step::proc