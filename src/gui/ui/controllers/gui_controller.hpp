#pragma once

#include <video/frame/interfaces/frame_interfaces.hpp>

#include <QObject>

#include <memory>

namespace step::gui {

class VideoFrameProvider;

class GuiController : public QObject
{
    Q_OBJECT

public:
    explicit GuiController(QObject* parent = nullptr);
    ~GuiController() override;

    video::IFrameSourceObserver* get_frame_observer() const;

    //спрашиваем, можно ли закрыть программу, выполняем всякие действия по закрытию
    Q_INVOKABLE bool main_window_closing();
    Q_INVOKABLE void start_video();

public slots:
    void show_main_window_slot();

    void on_video_frame_updated_slot();

private:
    void register_qml_types();
    void set_main_qml_engine();
    void show_windows_on_start();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace step::gui