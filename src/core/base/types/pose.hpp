#pragma once

#include "noise_settings.hpp"

#include <fmt/format.h>

#include <Eigen/Geometry>

namespace step {

class Pose : public ISerializable
{
public:
    Pose() = default;
    Pose(double _px, double _py, double _pz, double _qw, double _qx, double _qy, double _qz);
    Pose(const Eigen::Vector3d& p, const Eigen::Quaterniond& q);
    explicit Pose(const Eigen::Affine3d& affine);

    Eigen::Vector3d p() const noexcept;
    Eigen::Quaterniond q() const noexcept;
    Eigen::Affine3d affine3D() const noexcept;

    void set_translation(double x, double y, double z);
    void set_translation(const Eigen::Vector3d& p);

    void set_rotation(double w, double x, double y, double z);
    void set_rotation(const Eigen::Quaterniond& q);

    bool operator==(const Pose& rhs) const noexcept;
    bool operator!=(const Pose& rhs) const noexcept { return !(*this == rhs); }

    void serialize(ObjectPtrJSON&) override;
    void deserialize(const ObjectPtrJSON&) override;

private:
    void init(double _px, double _py, double _pz, double _qw, double _qx, double _qy, double _qz);

public:
    // translation
    double px = 0.;
    double py = 0.;
    double pz = 0.;

    // quaternion
    double qw = 0.;
    double qx = 0.;
    double qy = 0.;
    double qz = 0.;
};

}  // namespace step

namespace step {

Eigen::Affine3d affine_from_pq(const Eigen::Vector3d& p, const Eigen::Quaterniond& q);

Pose noise_pose(const Pose& pose, const NoiseSettings& noise);

}  // namespace step

template <>
struct fmt::formatter<Eigen::Affine3d> : formatter<string_view>
{
    template <typename FormatContext>
    auto format(const Eigen::Affine3d& affine, FormatContext& ctx)
    {
        const Eigen::Quaterniond q(affine.linear());
        const Eigen::Vector3d p(affine.translation());
        return fmt::format_to(ctx.out(),
                              "\np: {{{: 3.6f}, {: 3.6f}, {: 3.6f}}}"
                              "\nq: {{{: 3.6f}, {: 3.6f}, {: 3.6f}, {: 3.6f}}}",
                              p.x(), p.y(), p.z(), q.w(), q.x(), q.y(), q.z());
    }
};

template <>
struct fmt::formatter<step::Pose> : fmt::formatter<string_view>
{
    template <typename FormatContext>
    auto format(const step::Pose& pose, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(),
                              "\np: {{{: 3.6f}, {: 3.6f}, {: 3.6f}}}"
                              "\nq: {{{: 3.6f}, {: 3.6f}, {: 3.6f}, {: 3.6f}}}",
                              pose.px, pose.py, pose.pz, pose.qw, pose.qx, pose.qy, pose.qz);
    }
};
