#pragma once

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

namespace step::gui {

class VideoFrameProvider : public QObject, public video::IFrameSourceObserver
{
    Q_OBJECT

public:
    static void register_qml_type();

public:
    VideoFrameProvider(QObject* parent = nullptr);
    ~VideoFrameProvider();

    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ videoSurface WRITE setVideoSurface)

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