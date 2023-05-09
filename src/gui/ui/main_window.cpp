#include "main_window.hpp"

#include <QApplication>
#include <QFontDatabase>
#include <QtQml>
#include <QQmlApplicationEngine>
#include <QWindow>

#include <core/log/log.hpp>

namespace step::gui {

struct MainWindow::Impl
{
    explicit Impl() {}

    QQmlApplicationEngine engine;
    QPointer<QWindow> window;
};

MainWindow::MainWindow() : m_impl(std::make_unique<Impl>()) {}

MainWindow::~MainWindow() = default;

bool MainWindow::load()
{
    if (is_loaded())
    {
        return true;
    }

    const QString fontPath{":/fonts/RobotoMono-Medium.ttf"};
    int id = QFontDatabase::addApplicationFont(fontPath);
    if (id == -1)
    {
        STEP_LOG(L_CRITICAL, "Can't load font from {}", fontPath.toStdString());
        return false;
    }
    QString font = QFontDatabase::applicationFontFamilies(id).at(0);
    QApplication::setFont(QFont{font});

    QQmlContext* ctx = m_impl->engine.rootContext();

    // QStringLiteral could not be moved automatically: performance issue fixed in Qt 5.15
    m_impl->engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));  // NOLINT(performance-no-automatic-move)
    if (m_impl->engine.rootObjects().isEmpty())
    {
        STEP_LOG(L_CRITICAL, "Can't load GUI");
        return false;
    }

    QObject* rootObject = m_impl->engine.rootObjects().first();
    m_impl->window = qobject_cast<QWindow*>(rootObject);
    if (m_impl->window == nullptr)
    {
        STEP_LOG(L_CRITICAL, "Can't find root window");
        return false;
    }

    constexpr int guiX = 0;
    constexpr int guiY = 0;
    constexpr int guiW = 1024;
    constexpr int guiH = 768;
    m_impl->window->setGeometry(guiX, guiY, guiW, guiH);
    m_impl->window->hide();

    return true;
}

bool MainWindow::is_loaded() { return m_impl->window; }

void MainWindow::show()
{
    if (!is_loaded())
        return;

    m_impl->window->show();
    m_impl->window->raise();
}

void MainWindow::hide()
{
    if (is_loaded())
        m_impl->window->hide();
}

}  // namespace step::gui