#include "fake_camera.hpp"

#include <video/frame/utils/frame_utils.hpp>

#include <base/utils/json/json_utils.hpp>
#include <base/utils/exception/assert.hpp>

#include <log/log.hpp>

#include <stdexcept>

namespace {

template <typename ErrorCallable>
void add_frame(std::deque<std::filesystem::path>& frames, const std::filesystem::path& path, ErrorCallable handle_error)
{
    auto frame = step::video::utils::open_file(path.string(), step::video::PixFmt::BGR);
    if (!frame.is_valid())
    {
        handle_error(path);
        return;
    }
    frames.push_back(path);
}

}  // namespace

namespace step::video {

FakeFrameProvider::FakeFrameProvider(const FakeCameraSettings& settings) : m_settings(settings)
{
    if (!m_settings.frame_rate.has_value())
        m_settings.frame_rate = DEFAULT_FPS;

    const auto& source_path = m_settings.source_path;

    STEP_ASSERT(std::filesystem::exists(source_path),
                "Failed to create FakeFrameProvider: provided path {} does not exist", source_path.string());

    if (std::filesystem::is_directory(source_path))
    {
        STEP_LOG(L_INFO, "Camera {}: Searching for frames in directory {}", source_path.string(), m_settings.id);
        for (const auto& entry : std::filesystem::directory_iterator(source_path))
        {
            if (std::filesystem::is_directory(entry))  // skip directories
                continue;

            const auto& entry_path = entry.path();
            add_frame(m_frames, entry_path, [cam_id = m_settings.id](const std::filesystem::path& path) {
                STEP_LOG(L_DEBUG, "Camera {}: Path {} does not point to a valid frame", path.string(), cam_id);
            });
        }

        STEP_ASSERT(!m_frames.empty(), "Directory {} does not contain any valid frames", source_path.string());
    }
    else  // assume single file
    {
        add_frame(m_frames, source_path, [](const std::filesystem::path& path) {
            step::utils::throw_runtime_with_log(fmt::format("Path {} does not point to a valid frame", path.string()));
        });
    }
}

Frame FakeFrameProvider::get_next_frame()
{
    if (exhausted())
    {
        STEP_ASSERT(m_settings.is_cyclic, "All frames are read");

        // refill
        using std::swap;
        swap(m_frames, m_retired_frames);
    }

    auto next_frame_path = m_frames.front();
    m_frames.pop_front();

    if (m_settings.is_cyclic)
    {
        // clone the frame for later refilling
        m_retired_frames.push_back(next_frame_path);
    }

    // open_file() must succeed since each path is tested in the ctor
    return utils::open_file(next_frame_path.string(), m_settings.pix_fmt);
}

void FakeCameraStreamer::start()
{
    // TODO: ABA problem? maybe needs CAS
    {
        STEP_ASSERT(!m_is_streaming, "Fake camera {} is already running", m_provider.settings().id);
        m_is_streaming.store(true);
    }

    STEP_LOG(L_INFO, "Camera {}: Starting streaming...", m_provider.settings().id);
    // sleep according to the specified FPS
    const std::int64_t sleep = 1000.0 / static_cast<double>(m_provider.settings().frame_rate.value());
    const bool is_cyclic = m_provider.settings().is_cyclic;
    m_streamer_thread = std::thread(&FakeCameraStreamer::stream, this, std::chrono::milliseconds(sleep), is_cyclic);
}

void FakeCameraStreamer::stop()
{
    m_is_streaming.store(false);
    // TODO: we might want to have a separate function for waiting?
    if (m_streamer_thread.joinable())
    {
        m_streamer_thread.join();
    }
}

void FakeCameraStreamer::stream(std::chrono::milliseconds sleep_ms, bool is_cyclic)
{
    while (m_is_streaming)
    {
        Frame frame;
        {  // operations under provider mutex
            std::lock_guard g{m_provider_mutex};

            if (!m_is_streaming)  // re-test, now inside the mutexed block
                break;

            if (!is_cyclic && m_provider.exhausted())
            {
                STEP_LOG(L_INFO, "Camera {}: No more frames to read, stopping streaming...", m_provider.settings().id);
                m_is_streaming.store(false);
                break;
            }

            frame = Frame{m_provider.get_next_frame()};
        }

        handle_frame(std::move(frame));

        std::this_thread::sleep_for(sleep_ms);
    }
}

bool FakeCameraStreamer::exhausted() const
{
    const bool is_cyclic = m_provider.settings().is_cyclic;
    if (is_cyclic)
    {
        return false;
    }

    std::lock_guard g{m_provider_mutex};
    return m_provider.exhausted();
}

// Camera API:

std::unique_ptr<ICameraSingleshot> create_camera_singleshot(const FakeCameraSettings& settings)
{
    return std::make_unique<FakeCameraSingleshot>(settings);
}

std::unique_ptr<ICameraStreaming> create_camera_streaming(const FakeCameraSettings& settings)
{
    return std::make_unique<FakeCameraStreamer>(settings);
}

}  // namespace step::video