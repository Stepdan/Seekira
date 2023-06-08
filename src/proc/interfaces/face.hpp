#pragma once

#include <core/base/types/rect.hpp>

#include <video/frame/interfaces/frame.hpp>

#include <memory>
#include <vector>

namespace step::proc {

class IFace;
using FacePtr = std::shared_ptr<IFace>;
using Faces = std::vector<FacePtr>;

// TODO Что если где-то будет не float?
using FaceRecognizerData = std::vector<float>;
using FaceLandmarks = std::vector<Point2D>;

class IFace
{
public:
    virtual ~IFace() = default;

    virtual Rect get_rect() const noexcept = 0;
    virtual video::FramePtr get_frame() const noexcept = 0;
    virtual FaceLandmarks get_landmarks() const noexcept = 0;
    virtual FaceRecognizerData get_recognizer_data() const noexcept = 0;

    virtual void set_rect(const Rect& value) = 0;
    virtual void set_frame(const video::FramePtr& value) = 0;
    virtual void set_landmarks(const FaceLandmarks& value) = 0;
    virtual void set_recognizer_data(const FaceRecognizerData& value) = 0;
};

template <typename TImplType>
class BaseFace : public IFace
{
protected:
    using ImplType = TImplType;

public:
    Rect get_rect() const noexcept override { return m_rect; }
    video::FramePtr get_frame() const noexcept override { return m_frame; }
    FaceLandmarks get_landmarks() const noexcept override { return m_landmarks; }
    FaceRecognizerData get_recognizer_data() const noexcept override { return m_recognizer_data; }

    bool is_empty() const noexcept
    {
        /* clang-format off */
        return true
            && m_rect.is_valid()
            && !m_frame
            && m_landmarks.empty()
            && m_recognizer_data.empty()
        ;
        /* clang-format on */
    }

protected:
    BaseFace() = default;

    std::shared_ptr<ImplType> get_impl_data() const { return m_impl; }
    void set_impl_data(std::shared_ptr<ImplType>&& value) { m_impl = value; }

    void set_rect(const Rect& value) override { m_rect = value; }
    void set_frame(const video::FramePtr& value) override { m_frame = value; }
    void set_landmarks(const FaceLandmarks& value) override { m_landmarks = value; }
    void set_recognizer_data(const FaceRecognizerData& value) override { m_recognizer_data = value; }

protected:
    Rect m_rect;
    video::FramePtr m_frame;
    FaceLandmarks m_landmarks;
    FaceRecognizerData m_recognizer_data;

    std::shared_ptr<ImplType> m_impl{nullptr};
};

}  // namespace step::proc