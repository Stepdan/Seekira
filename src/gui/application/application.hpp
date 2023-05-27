#pragma once

#include <QApplication>

#include <memory>

class QMainWindow;

namespace step::gui {

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    virtual ~Application();

    /// @brief Методы используются в QML приложениях, для того чтобы устанавливать родителя QDialog
    void set_main_window(QMainWindow* main_window);
    QWidget* get_main_window();

    void start();
    bool is_started() const;

    /// @brief Попытка выхода из приложения
    /// Выставляет флаг IsQuitting()
    /// @return разрешен ли выход
    bool try_to_quit();
    bool is_quitting() const;

protected:
    /// @brief Можно здесь запретить выход, разместить опрос пользователя на выход,
    /// закрытие всех окон приложения и тд
    virtual bool try_to_quit_impl();

    virtual void on_start();

protected slots:
    void on_about_to_quit_slot();

signals:
    void application_started_signal();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

#define step_app (static_cast<step::gui::Application*>(QApplication::instance()))

}  // namespace step::gui