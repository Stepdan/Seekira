#include "main_window.hpp"

#include <core/log/log.hpp>
#include <core/exception/assert.hpp>

#include <gui/application/application.hpp>
#include <gui/interfaces/objects_connector_id.hpp>

#include <QApplication>
#include <QFontDatabase>
#include <QtQml>
#include <QQmlApplicationEngine>
#include <QWindow>
#include <QMainWindow>

namespace step::gui {

struct MainWindow::Impl : public QObject
{
    QMainWindow m_main_window;
};

MainWindow::MainWindow() : m_impl(std::make_unique<Impl>())
{
    utils::ObjectsConnector::register_receiver(ObjectsConnectorID::SET_QML_IN_MAIN_WINDOW(), this,
                                               SLOT(on_set_qml_window_in_main_window_slot()));
}

MainWindow::~MainWindow() = default;

MainWindow::operator QMainWindow*() const { return &m_impl->m_main_window; }

void MainWindow::on_set_qml_window_in_main_window_slot()
{
    STEP_LOG(L_DEBUG, "on_set_qml_window_in_main_window_slot started");

    const auto windows = step_app->allWindows();
    const auto it = std::find_if(std::cbegin(windows), std::cend(windows), [](const auto* window) {
        return window->objectName() == ObjectsConnectorID::QML_MAIN_WINDOW();
    });
    STEP_ASSERT(it != std::cend(windows), "No MainWindow in Application!");

    const auto* window = *it;
    STEP_ASSERT(window, "Invalid MainWindow!");

    const auto x = window->x();
    const auto y = window->y();
    const auto width = window->width();
    const auto height = window->height();

    m_impl->m_main_window.setGeometry(QRect(QPoint(x, y), QPoint(x + width, y + height)));
    STEP_LOG(L_TRACE, "MainWindow geometry: x={}, y={}, width={}, height={}", x, y, width, height);

    const auto set_pos = [](QMainWindow& window, int x, int y) {
        window.setGeometry(x, y, window.width(), window.height());
    };

    connect(window, &QWindow::xChanged, [&w = m_impl->m_main_window, set_pos](int x) { set_pos(w, x, w.y()); });
    connect(window, &QWindow::yChanged, [&w = m_impl->m_main_window, set_pos](int y) { set_pos(w, w.x(), y); });
}

}  // namespace step::gui