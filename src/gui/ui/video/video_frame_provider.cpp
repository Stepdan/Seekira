#include "video_frame_provider.hpp"

#include <core/log/log.hpp>

#include <gui/utils/qimage_utils.hpp>

#include <QtQml/qqml.h>

namespace step::gui {

void VideoFrameProvider::register_qml_type()
{
    qmlRegisterType<VideoFrameProvider>("step.VideoFrameProvider", 1, 0, "VideoFrameProvider");
}

VideoFrameProvider::VideoFrameProvider(QObject* parent /*= nullptr*/) : QObject(parent), m_surface(nullptr) {}

VideoFrameProvider::~VideoFrameProvider() { closeSurface(); }

QAbstractVideoSurface* VideoFrameProvider::videoSurface() const { return m_surface; }

void VideoFrameProvider::setVideoSurface(QAbstractVideoSurface* surface)
{
    closeSurface();
    m_surface = surface;
}

void VideoFrameProvider::closeSurface()
{
    if (m_surface && m_surface->isActive())
        m_surface->stop();
}

void VideoFrameProvider::process_frame(video::FramePtr frame_ptr)
{
    if (!frame_ptr)
    {
        STEP_LOG(L_ERROR, "Empty frame in VideoFrameProvider::process_frame");
        return;
    }

    auto image = utils::frame_to_qimage_deep(*frame_ptr);
    QVideoFrame::PixelFormat pixel_format = QVideoFrame::pixelFormatFromImageFormat(image.format());

    //если формат кадра по какой-то причине поменялся (или это первый кадр)-
    //выполним повторную (первичную) инициализацию surface
    if (image.size() != m_format.frameSize() || pixel_format != m_format.pixelFormat())
    {
        closeSurface();
        auto format = QVideoSurfaceFormat(image.size(), pixel_format);
        if (format.isValid())
        {
            m_format = format;
            m_surface->start(m_format);
        }
        else
        {
            STEP_LOG(L_ERROR, "Invalid surface format: {}", m_format.pixelFormat());
            return;
        }
    }

    STEP_LOG(L_INFO, "Frame processing: {}", frame_ptr);
    //передадим полученный кадр на отрисовку
    m_surface->present(QVideoFrame(image));
}

}  // namespace step::gui