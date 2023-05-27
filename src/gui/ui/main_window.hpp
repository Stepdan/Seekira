#pragma once

#include <QMainWindow>

#include <memory>

namespace step::gui {

class MainWindow : public QObject
{
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow() override;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

public:
    operator QMainWindow*() const;

private slots:
    void on_set_qml_window_in_main_window_slot();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace step::gui