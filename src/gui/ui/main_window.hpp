#pragma once

#include <QObject>

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
    bool load();
    bool is_loaded();

    void show();
    void hide();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}  // namespace step::gui