#include "gui_controller.hpp"
#include "player_controller_ff.hpp"

#include <core/log/log.hpp>

#include <core/exception/assert.hpp>

#include <gui/interfaces/objects_connector_id.hpp>

#include <QAbstractItemModel>
#include <QTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>

// GuiController::Impl
namespace step::gui {

struct GuiController::Impl
{
    Impl(GuiController& gc) : m_gui_controller(gc)
    {
        m_player_controller = std::make_unique<PlayerControllerFF>(&m_gui_controller);
    }

    ~Impl()
    {
        // Чтобы не утекали QMLные объекты
        m_engine->clearComponentCache();
    }

public:
    std::unique_ptr<QQmlApplicationEngine> m_engine;

    std::unique_ptr<IPlayerController> m_player_controller;

private:
    GuiController& m_gui_controller;
};

}  // namespace step::gui

// GuiController
namespace step::gui {

GuiController::GuiController(QObject* parent) : QObject(parent), m_impl(std::make_unique<Impl>(*this))
{
    register_meta_types();
    register_qml_types();

    STEP_LOG(L_INFO, "GuiController has been created!");
}

GuiController::~GuiController() {}

bool GuiController::main_window_closing() { return true; }

void GuiController::show_main_window_slot()
{
    if (m_impl->m_engine)
        return;

    std::make_unique<QQmlApplicationEngine>().swap(m_impl->m_engine);
    STEP_LOG(L_DEBUG, "QQmlApplicationEngine created");

    set_main_qml_engine();
    STEP_LOG(L_DEBUG, "set_main_qml_engine done");

    show_windows_on_start();
}

void GuiController::register_qml_types()
{
    //регистрация всех классов C++, требующих доступа из QML
    register_qml_enums();

    IVideoFrameProvider::register_qml_type();

    qmlRegisterInterface<QAbstractItemModel>("QAbstractItemModel");
    qmlRegisterInterface<IPlayerController>("IPlayerController");
}

void GuiController::register_meta_types() {}

void GuiController::set_main_qml_engine()
{
    // установка глобальных объектов, доступных из qml
    STEP_ASSERT(m_impl->m_engine, "QML engine is empty!");

    //объекты со статическими свойствами/методами
    const auto qml_context = m_impl->m_engine->rootContext();
    STEP_LOG(L_DEBUG, "qml_context extracted");

    // cp - context property
    qml_context->setContextProperty("cpGuiController", this);
    qml_context->setContextProperty("cpObjectsConnectorID", new step::gui::ObjectsConnectorID(this));
    qml_context->setContextProperty("cpObjectsConnector", new step::gui::utils::ObjectsConnector(this));
    qml_context->setContextProperty("cpVideoFrameProvider", m_impl->m_player_controller->get_video_frame_provider());
    qml_context->setContextProperty("cpPlayerController", m_impl->m_player_controller.get());
    STEP_LOG(L_DEBUG, "qml_context setContextProperty done");

    m_impl->m_engine->load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    STEP_LOG(L_DEBUG, "main.qml loaded");
}

void GuiController::show_windows_on_start()
{
    QTimer::singleShot(0, [this]() {
        // Show windows on program start
    });
}

}  // namespace step::gui