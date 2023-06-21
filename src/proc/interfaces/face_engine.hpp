#pragma once

#include "face.hpp"

#include <proc/interfaces/face_engine_type.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <filesystem>

namespace step::proc {

class IFaceEngine
{
public:
    enum Mode
    {
        /* clang-format off */
        FE_UNDEFINED   = (1u << 0),
        FE_DETECTION   = (1u << 1),
        FE_LANDMARKS   = (1u << 2),
        FE_RECOGNITION = (1u << 3),
        FE_DETECTION_RECOGNITION = FE_DETECTION | FE_RECOGNITION,
        FE_ALL = FE_DETECTION | FE_LANDMARKS | FE_RECOGNITION,
        /* clang-format on */
    };

    struct Initializer : public ISerializable
    {
        FaceEngineType type{FaceEngineType::Undefined};
        Mode mode{FE_UNDEFINED};
        std::filesystem::path models_path;

        bool save_frames = false;  // Обрезка кадра и сохранение в IFace

        void deserialize(const ObjectPtrJSON& container);

        bool is_valid() const noexcept;

        bool operator==(const Initializer& rhs) const noexcept;
        bool operator!=(const Initializer& rhs) const noexcept { return !(*this == rhs); }
    };

public:
    virtual ~IFaceEngine() = default;

    virtual Faces detect(const video::Frame& frame) = 0;
    virtual void recognize(const FacePtr&) = 0;
    virtual bool compare(const FacePtr&, const FacePtr&) = 0;

protected:
    virtual void calc_landmarks(const video::Frame&, const FacePtr&) = 0;

protected:
    virtual bool load_models() = 0;
};

class BaseFaceEngine : public IFaceEngine
{
protected:
    BaseFaceEngine(IFaceEngine::Initializer&& init)
        : m_mode(std::move(init.mode))
        , m_models_path(std::move(init.models_path))
        , m_save_frames(std::move(init.save_frames))
    {
    }

protected:
    IFaceEngine::Mode m_mode;
    std::filesystem::path m_models_path;
    bool m_save_frames = false;  // Обрезка кадра и сохранение в IFace
};

}  // namespace step::proc