#include "video_frame_provider.hpp"

#include <core/log/log.hpp>

#include <gui/utils/qimage_utils.hpp>
#include <gui/utils/log_handler.hpp>

#include <gui/interfaces/declare_metatype.hpp>
#include <gui/interfaces/objects_connector_id.hpp>

#include <QtQml/qqml.h>

namespace step::gui {

void VideoFrameProvider::register_qml_type()
{
    qmlRegisterType<VideoFrameProvider>("step.VideoFrameProvider", 1, 0, "VideoFrameProvider");
}

VideoFrameProvider::VideoFrameProvider(QObject* parent /*= nullptr*/) : QObject(parent), m_surface(nullptr)
{
    // QueuedConnection, чтобы выполнить обработку в GUI потоке
    connect(this, &VideoFrameProvider::frame_updated_signal, this, &VideoFrameProvider::on_process_frame_slot,
            Qt::ConnectionType::QueuedConnection);
}

VideoFrameProvider::~VideoFrameProvider() { close_surface(); }

QAbstractVideoSurface* VideoFrameProvider::video_surface() const
{
    std::scoped_lock(m_guard);
    return m_surface;
}

void VideoFrameProvider::set_video_surface(QAbstractVideoSurface* surface)
{
    std::scoped_lock(m_guard);
    close_surface();
    m_surface = surface;
}

void VideoFrameProvider::close_surface()
{
    if (m_surface && m_surface->isActive())
        m_surface->stop();
}

void VideoFrameProvider::process_frame(step::video::FramePtr frame_ptr)
{
    // Шлем сигнал, чтобы выполнить обработку в GUI потоке
    emit frame_updated_signal(frame_ptr);
}

void VideoFrameProvider::on_process_frame_slot(const step::video::FramePtr& frame_ptr)
{
    try
    {
        if (!frame_ptr)
        {
            STEP_LOG(L_ERROR, "Empty frame in VideoFrameProvider::process_frame");
            return;
        }

        auto image = utils::frame_to_qimage_deep(*frame_ptr);
        if (image.isNull())
            return;

        QVideoFrame::PixelFormat pixel_format = QVideoFrame::pixelFormatFromImageFormat(image.format());

        // принудительно конвертируем в 100% рабочий формат для surface
        if (pixel_format == QVideoFrame::PixelFormat::Format_Invalid)
        {
            image.convertTo(QImage::Format::Format_RGB32);
            pixel_format = QVideoFrame::pixelFormatFromImageFormat(image.format());
        }

        std::scoped_lock(m_guard);
        // если формат кадра по какой-то причине поменялся (или это первый кадр)-
        // выполним повторную (первичную) инициализацию surface
        if (image.size() != m_format.frameSize() || pixel_format != m_format.pixelFormat())
        {
            STEP_LOG(L_INFO, "Try to init surface format with frame: {}", frame_ptr);
            close_surface();
            auto format = QVideoSurfaceFormat(image.size(), pixel_format);
            if (format.isValid())
            {
                m_format = format;
                m_surface->start(m_format);
            }
            else
            {
                STEP_LOG(L_ERROR, "Invalid surface format: {}", format.pixelFormat());
                return;
            }
        }

        STEP_LOG(L_TRACE, "VideoFrameProvider: Frame processing: {}", frame_ptr);
        if (!m_surface->present(QVideoFrame(image)))
        {
            STEP_LOG(L_ERROR, "Failed to present frame");
            return;
        }
    }
    catch (std::exception& e)
    {
        STEP_LOG(L_ERROR, "Catch exception during VideoFrameProvider::process_frame: {}", e.what());
        m_exception = std::current_exception();
    }
    catch (...)
    {
        STEP_LOG(L_ERROR, "Catch unknown exception during VideoFrameProvider::process_frame:");
        m_exception = std::current_exception();
    }
}

}  // namespace step::gui