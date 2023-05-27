#pragma once

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

namespace step::gui {

class VideoFrameProvider : public QObject, video::IFrameSourceObserver
{
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ videoSurface WRITE setVideoSurface)

public:
    VideoFrameProvider(QObject* parent);
    ~VideoFrameProvider();

public:
    QAbstractVideoSurface* videoSurface() const;
    void setVideoSurface(QAbstractVideoSurface* surface);

private:
    void closeSurface();

    void process_frame(video::FramePtr frame_ptr);

private:
    QAbstractVideoSurface* m_surface;
    QVideoSurfaceFormat m_format;
};

}  // namespace step::gui