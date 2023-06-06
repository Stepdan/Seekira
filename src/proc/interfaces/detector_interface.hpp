#pragma once

#include <core/base/types/meta_storage.hpp>

#include <core/task/base_task.hpp>

#include <video/frame/interfaces/frame.hpp>

namespace step::proc {

struct DetectionArea
{
    int x0{-1}, y0{-1};
    int x1{-1}, y1{-1};

    DetectionArea() = default;
    DetectionArea(int _x0, int _y0, int _x1, int _y1) : x0(_x0), y0(_y0), x1(_x1), y1(_y1) {}
};

class DetectionResult
{
public:
    DetectionResult() = default;
    DetectionResult(DetectionArea&& area, MetaStorage&& data) : m_area(std::move(area)), m_data(std::move(data)) {}

    DetectionArea area() const noexcept { return m_area; }
    MetaStorage data() const noexcept { return m_data; }

private:
    DetectionArea m_area;
    MetaStorage m_data;
};

using IDetector = task::ITask<video::Frame, DetectionResult>;

template <typename TSettings>
using BaseDetector = task::BaseTask<TSettings, video::Frame, DetectionResult>;

}  // namespace step::proc