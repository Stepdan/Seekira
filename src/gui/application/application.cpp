#include "application.hpp"

#include <core/exception/assert.hpp>

#include <gui/utils/spontaneous_dialog.hpp>

#include <QAbstractScrollArea>
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QTimer>
#include <QMainWindow>  // for QMainWindow->QWidget cast
#include <QCommandLineOption>
#include <QMetaEnum>

// Application::Impl
namespace step::gui {

struct Application::Impl
{
public:
    Impl(Application*) {}

    ///< Флаг того, что приложение уже запущено, используется для фильтрации некоторых событий в notify
    bool m_started{false};
    ///< Флаг выхода из приложения, выставляется по результатом вызова try_to_quit
    bool m_is_quitting{false};
};

}  // namespace step::gui

// Application
namespace step::gui {

Application::Application(int& argc, char** argv) : QApplication(argc, argv), m_impl(std::make_unique<Impl>(this))
{
    // анимация заметно моргает под Windows и создает проблемы
    // с ресайзом view в AdvancedComboBox
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(on_about_to_quit_slot()));
}

Application::~Application() {}

void Application::set_main_window(QMainWindow* main_window)
{
    STEP_ASSERT(main_window, "Invalid main_window!");
    utils::spontaneous::set_main_window(main_window);
}

QWidget* Application::get_main_window() { return utils::spontaneous::get_main_window(); }

void Application::start()
{
    if (m_impl->m_started)
        return;

    m_impl->m_started = true;

    on_start();
}

bool Application::is_started() const { return m_impl->m_started; }

void Application::on_start() { emit application_started_signal(); }

bool Application::try_to_quit()
{
    m_impl->m_is_quitting = try_to_quit_impl();
    return is_quitting();
}

bool Application::is_quitting() const { return m_impl->m_is_quitting; }

bool Application::try_to_quit_impl()
{
    quit();
    return true;
}

void Application::on_about_to_quit_slot() {}

}  // namespace step::gui