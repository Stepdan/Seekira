#pragma once

#include <video/camera/camera_fake/fake_camera_settings.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <atomic>
#include <deque>
#include <filesystem>
#include <thread>
#include <mutex>

namespace step::video {

class FakeFrameProvider
{
public:
    static constexpr int DEFAULT_FPS = 30;

    FakeFrameProvider(const FakeCameraSettings& params);

    Frame get_next_frame();
    bool exhausted() const { return m_frames.empty(); }
    FakeCameraSettings settings() const { return m_settings; }

private:
    std::deque<std::filesystem::path> m_frames;
    std::deque<std::filesystem::path> m_retired_frames;
    FakeCameraSettings m_settings;
};

class FakeCameraSingleshot : public ICameraSingleshot
{
public:
    FakeCameraSingleshot(const FakeCameraSettings& extra) : m_provider(extra) {}

    Frame get_frame(Milliseconds) override { return Frame{m_provider.get_next_frame()}; }

    bool exhausted() const { return !m_provider.settings().is_cyclic && m_provider.exhausted(); }

private:
    FakeFrameProvider m_provider;
};

class FakeCameraStreamer : public ICameraStreaming
{
public:
    FakeCameraStreamer(const FakeCameraSettings& params) : m_provider(params) {}
    ~FakeCameraStreamer() { stop(); }

    void start() override;
    void stop() override;
    bool exhausted() const;

private:
    void stream(std::chrono::milliseconds sleep_ms, bool is_cyclic);

    FakeFrameProvider m_provider;
    mutable std::mutex m_provider_mutex;

    std::thread m_streamer_thread{};
};

}  // namespace step::video
