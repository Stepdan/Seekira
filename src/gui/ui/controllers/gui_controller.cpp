#include "gui_controller.hpp"

#include <core/log/log.hpp>

#include <core/exception/assert.hpp>

#include <gui/interfaces/objects_connector_id.hpp>

#include <gui/ui/video/video_frame_provider.hpp>

#include <video/ffmpeg/reader/reader.hpp>

#include <QAbstractItemModel>
#include <QTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>

// GuiController::Impl
namespace step::gui {

struct GuiController::Impl
{
    Impl(GuiController& gc) : m_gui_controller(gc) { m_video_frame_provider.setParent(&m_gui_controller); }

    ~Impl()
    {
        // Чтобы не утекали QMLные объекты
        m_engine->clearComponentCache();
    }

public:
    std::unique_ptr<QQmlApplicationEngine> m_engine;

    VideoFrameProvider m_video_frame_provider;
    std::unique_ptr<video::ff::ReaderFF> m_video_reader;

private:
    GuiController& m_gui_controller;
};

}  // namespace step::gui

// GuiController
namespace step::gui {

GuiController::GuiController(QObject* parent) : QObject(parent), m_impl(std::make_unique<Impl>(*this))
{
    register_qml_types();

    connect(&m_impl->m_video_frame_provider, &VideoFrameProvider::frame_updated_signal, this,
            &GuiController::on_video_frame_updated_slot);

    STEP_LOG(L_INFO, "GuiController has been created!");
}

GuiController::~GuiController() {}

video::IFrameSourceObserver* GuiController::get_frame_observer() const { return &(m_impl->m_video_frame_provider); }

bool GuiController::main_window_closing() { return true; }

void GuiController::start_video()
{
    STEP_LOG(L_DEBUG, "GuiController: Start video");
    m_impl->m_video_reader = std::make_unique<video::ff::ReaderFF>();
    m_impl->m_video_reader->open_file("C:/Work/test_video/IMG_5903.MOV");
    m_impl->m_video_reader->register_observer(get_frame_observer());
    m_impl->m_video_reader->start(video::ff::ReadingMode::Continuously);
}

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

void GuiController::on_video_frame_updated_slot() {}

void GuiController::register_qml_types()
{
    //регистрация всех классов C++, требующих доступа из QML
    VideoFrameProvider::register_qml_type();
    m_impl->m_video_frame_provider.setObjectName("videoFrameProvider");

    qmlRegisterInterface<QAbstractItemModel>("QAbstractItemModel");
}

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
    qml_context->setContextProperty("cpVideoFrameProvider", &m_impl->m_video_frame_provider);
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