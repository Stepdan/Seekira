#pragma once

#include <video/camera/interfaces/camera_factory.hpp>

#include <base/interfaces/serializable.hpp>

#include <filesystem>

namespace step::video {

struct FakeCameraSettings : public CameraSettings
{
    std::filesystem::path source_path{};
    bool is_cyclic{false};

    FakeCameraSettings() = default;
    FakeCameraSettings(const std::filesystem::path& p) : source_path(p) {}
    FakeCameraSettings(const std::filesystem::path& p, bool cyclic) : source_path(p), is_cyclic(cyclic) {}

    bool operator==(const FakeCameraSettings& rhs) const noexcept;
    bool operator!=(const FakeCameraSettings& rhs) const noexcept { return !(*this == rhs); }

    void serialize(ObjectPtrJSON&) override;
    void deserialize(const ObjectPtrJSON&) override;
};

std::unique_ptr<ICameraSingleshot> create_camera_singleshot(const FakeCameraSettings&);
std::unique_ptr<ICameraStreaming> create_camera_streaming(const FakeCameraSettings&);

}  // namespace step::video
