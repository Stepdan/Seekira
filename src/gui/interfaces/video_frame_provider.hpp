#pragma once

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include <mutex>

namespace step::gui {

class IVideoFrameProvider : public QObject
{
    Q_OBJECT

public:
    static void register_qml_type();

public:
    IVideoFrameProvider(QObject* parent = nullptr) : QObject(parent), m_surface(nullptr) {}
    virtual ~IVideoFrameProvider() { close_surface(); }

    Q_PROPERTY(QAbstractVideoSurface* videoSurface READ video_surface WRITE set_video_surface)

public:
    QAbstractVideoSurface* video_surface() const;
    void set_video_surface(QAbstractVideoSurface* surface);

protected:
    void close_surface();

protected:
    QAbstractVideoSurface* m_surface;
    QVideoSurfaceFormat m_format;

    std::mutex m_guard;
};

}  // namespace step::gui