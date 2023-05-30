#pragma once

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <gui/interfaces/video_frame_provider.hpp>

namespace step::gui {

class VideoFrameProviderFF : public IVideoFrameProvider, public video::IFrameSourceObserver
{
    Q_OBJECT
public:
    VideoFrameProviderFF(QObject* parent = nullptr);
    ~VideoFrameProviderFF();

private:
    void process_frame(video::FramePtr frame_ptr);

private slots:
    void on_process_frame_slot(const step::video::FramePtr& frame_ptr);

signals:
    void frame_updated_signal(const step::video::FramePtr& frame_ptr);

private:
    std::exception_ptr m_exception;
};

}  // namespace step::gui