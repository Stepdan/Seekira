#pragma once

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

    //спрашиваем, можно ли закрыть программу, выполняем всякие действия по закрытию
    Q_INVOKABLE bool main_window_closing();

public slots:
    void show_main_window_slot();

private:
    void register_qml_types();
    void register_meta_types();
    void set_main_qml_engine();
    void show_windows_on_start();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace step::gui