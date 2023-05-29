#pragma once

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include <mutex>

namespace step::gui {

class VideoFrameProvider : public QObject, public video::IFrameSourceObserver
{
    Q_OBJECT

public:
    static void register_qml_type();

public:
    VideoFrameProvider(QObject* parent = nullptr);
    ~VideoFrameProvider();

    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ video_surface WRITE set_video_surface)

public:
    QAbstractVideoSurface* video_surface() const;
    void set_video_surface(QAbstractVideoSurface* surface);

private:
    void close_surface();

    void process_frame(video::FramePtr frame_ptr);

private slots:
    void on_process_frame_slot(const step::video::FramePtr& frame_ptr);

signals:
    void frame_updated_signal(const step::video::FramePtr& frame_ptr);

private:
    QAbstractVideoSurface* m_surface;
    QVideoSurfaceFormat m_format;

    std::mutex m_guard;
    std::exception_ptr m_exception;
};

}  // namespace step::gui