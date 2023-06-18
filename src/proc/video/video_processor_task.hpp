#pragma once

#include <proc/interfaces/video_processor_interface.hpp>

namespace step::proc {

std::unique_ptr<IVideoProcessorTask> create_video_processor_task(const std::shared_ptr<task::BaseSettings>& settings);

}