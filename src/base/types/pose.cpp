#include "pose.hpp"

#include <base/types/config_fields.hpp>
#include <base/types/math_constants.hpp>
#include <base/utils/type_utils.hpp>

#include <random>

namespace step {

Pose::Pose(double _px, double _py, double _pz, double _qw, double _qx, double _qy, double _qz)
{
    init(_px, _py, _pz, _qw, _qx, _qy, _qz);
}

Pose::Pose(const Eigen::Vector3d& p, const Eigen::Quaterniond& q)
{
    init(p.x(), p.y(), p.z(), q.w(), q.x(), q.y(), q.z());
}

Pose::Pose(const Eigen::Affine3d& affine)
{
    const auto p = affine.translation();
    const auto q = Eigen::Quaterniond(affine.linear()).normalized();
    init(p.x(), p.y(), p.z(), q.w(), q.x(), q.y(), q.z());
}

bool Pose::operator==(const Pose& rhs) const noexcept
{
    /* clang-format off */
        return true
            && utils::compare(px, rhs.px)
            && utils::compare(py, rhs.py)
            && utils::compare(pz, rhs.pz)
            && utils::compare(qx, rhs.qx)
            && utils::compare(qy, rhs.qy)
            && utils::compare(qz, rhs.qz)
            && utils::compare(qw, rhs.qw)
        ;
    /* clang-format on */
}

Eigen::Vector3d Pose::p() const noexcept { return Eigen::Vector3d(px, py, pz); }

Eigen::Quaterniond Pose::q() const noexcept { return Eigen::Quaterniond(qw, qx, qy, qz); }

Eigen::Affine3d Pose::affine3D() const noexcept { return affine_from_pq(p(), q()); }

void Pose::set_translation(double x, double y, double z)
{
    px = x;
    py = y;
    pz = z;
}

void Pose::set_translation(const Eigen::Vector3d& p) { set_translation(p.x(), p.y(), p.z()); }

void Pose::set_rotation(double w, double x, double y, double z)
{
    qw = w;
    qx = x;
    qy = y;
    qz = z;
}

void Pose::set_rotation(const Eigen::Quaterniond& q) { set_rotation(q.w(), q.x(), q.y(), q.z()); }

void Pose::init(double _px, double _py, double _pz, double _qw, double _qx, double _qy, double _qz)
{
    set_translation(_px, _py, _pz);
    set_rotation(_qw, _qx, _qy, _qz);
}

}  // namespace step

namespace step {

Eigen::Affine3d affine_from_pq(const Eigen::Vector3d& p, const Eigen::Quaterniond& q)
{
    auto affine = Eigen::Affine3d::Identity();
    affine.translate(p);
    affine.rotate(q);

    return affine;
}

Pose noise_pose(const Pose& pose, const NoiseSettings& noise_settings)
{
    std::random_device rd;
    std::default_random_engine gen(rd());
    std::normal_distribution<double> tr_dist(0.0, noise_settings.translation);
    std::normal_distribution<double> rot_dist(0.0, noise_settings.angle);

    auto noise = affine_from_pq(
        tr_dist(gen) * Eigen::Vector3d::Random(),
        Eigen::Quaterniond(Eigen::AngleAxisd(rot_dist(gen) * math::M_PI / 180, Eigen::Vector3d::Random())));

    // Sometimes values could be bigger than limits.
    // We will saturate it.
    auto noised_pose = Pose(pose.affine3D() * noise);
    auto noised_tr = noised_pose.p();
    const auto pose_tr = pose.p();
    for (size_t i = 0; i < 3; ++i)
    {
        const auto diff = noised_tr[i] - pose_tr[i];
        if (std::fabs(diff) > noise_settings.translation)
            noised_tr[i] = pose_tr[i] + (diff > 0 ? 1 : -1) * noise_settings.translation;
    }
    noised_pose.set_translation(noised_tr);

    return noised_pose;
}

}  // namespace step

// Serialization
namespace step {

// Pose
void Pose::serialize(ObjectPtrJSON& container)
{
    auto p = json::make_object_json_ptr();
    auto q = json::make_object_json_ptr();
    json::set(p, CFG_FLD::X, px);
    json::set(p, CFG_FLD::Y, py);
    json::set(p, CFG_FLD::Z, pz);
    json::set(q, CFG_FLD::W, qw);
    json::set(q, CFG_FLD::X, qx);
    json::set(q, CFG_FLD::Y, qy);
    json::set(q, CFG_FLD::Z, qz);
    json::set(container, CFG_FLD::P, p);
    json::set(container, CFG_FLD::Q, q);
}

void Pose::deserialize(const ObjectPtrJSON& container)
{
    auto p = json::get_object(container, CFG_FLD::P);
    auto q = json::get_object(container, CFG_FLD::Q);
    set_translation(json::get<double>(p, CFG_FLD::X), json::get<double>(p, CFG_FLD::Y),
                    json::get<double>(p, CFG_FLD::Z));
    set_rotation(json::get<double>(q, CFG_FLD::W), json::get<double>(q, CFG_FLD::X), json::get<double>(q, CFG_FLD::Y),
                 json::get<double>(q, CFG_FLD::Z));
}

}  // namespace step
