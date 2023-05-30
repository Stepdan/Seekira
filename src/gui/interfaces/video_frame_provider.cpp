#include "video_frame_provider.hpp"

#include <QtQml/qqml.h>

namespace step::gui {

void IVideoFrameProvider::register_qml_type()
{
    qmlRegisterType<IVideoFrameProvider>("step.gui.VideoFrameProvider", 1, 0, "VideoFrameProvider");
}

QAbstractVideoSurface* IVideoFrameProvider::video_surface() const
{
    std::scoped_lock(m_guard);
    return m_surface;
}

void IVideoFrameProvider::set_video_surface(QAbstractVideoSurface* surface)
{
    std::scoped_lock(m_guard);
    close_surface();
    m_surface = surface;
}

void IVideoFrameProvider::close_surface()
{
    if (m_surface && m_surface->isActive())
        m_surface->stop();
}

}  // namespace step::gui