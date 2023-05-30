#include "video_frame_provider_ff.hpp"

#include <core/log/log.hpp>

#include <gui/utils/qimage_utils.hpp>
#include <gui/utils/log_handler.hpp>

#include <gui/interfaces/declare_metatype.hpp>
#include <gui/interfaces/objects_connector_id.hpp>

namespace step::gui {

VideoFrameProviderFF::VideoFrameProviderFF(QObject* parent /*= nullptr*/) : IVideoFrameProvider(parent)
{
    // QueuedConnection, чтобы выполнить обработку в GUI потоке
    connect(this, &VideoFrameProviderFF::frame_updated_signal, this, &VideoFrameProviderFF::on_process_frame_slot,
            Qt::ConnectionType::QueuedConnection);
}

VideoFrameProviderFF::~VideoFrameProviderFF() {}

void VideoFrameProviderFF::process_frame(step::video::FramePtr frame_ptr)
{
    // Шлем сигнал, чтобы выполнить обработку в GUI потоке
    emit frame_updated_signal(frame_ptr);
}

void VideoFrameProviderFF::on_process_frame_slot(const step::video::FramePtr& frame_ptr)
{
    try
    {
        if (!frame_ptr)
        {
            STEP_LOG(L_ERROR, "Empty frame in VideoFrameProviderFF::process_frame");
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

        STEP_LOG(L_TRACE, "VideoFrameProviderFF: Frame processing: {}", frame_ptr);
        if (!m_surface->present(QVideoFrame(image)))
        {
            STEP_LOG(L_ERROR, "Failed to present frame");
            return;
        }
    }
    catch (std::exception& e)
    {
        STEP_LOG(L_ERROR, "Catch exception during VideoFrameProviderFF::process_frame: {}", e.what());
        m_exception = std::current_exception();
    }
    catch (...)
    {
        STEP_LOG(L_ERROR, "Catch unknown exception during VideoFrameProviderFF::process_frame:");
        m_exception = std::current_exception();
    }
}

}  // namespace step::gui