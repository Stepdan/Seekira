#include <core/log/log.hpp>

#include <gui/utils/high_dpi_fix.hpp>
#include <gui/utils/log_handler.hpp>
#include <gui/application/application.hpp>

#include <gui/ui/controllers/gui_controller.hpp>
#include <gui/ui/main_window.hpp>

#include <QApplication>
#include <QGuiApplication>
#include <QQuickStyle>

/**
 * @brief Load resources from static library step::ui_resources
 * 
 */
struct QtResources
{
    QtResources() { Q_INIT_RESOURCE(resources); }
    ~QtResources() { Q_CLEANUP_RESOURCE(resources); }
} resources;

int main(int argc, char* argv[])
{
    int exit_code = -1;
    try
    {
#ifdef STEPKIT_DEBUG
        step::log::Logger::instance().set_log_level(L_DEBUG);
#endif

        step::gui::utils::high_dpi_fix();
        qInstallMessageHandler(step::gui::utils::qt_log_handler);

        step::gui::Application app(argc, argv);
        STEP_LOG(L_INFO, "Application created");

        step::gui::MainWindow main_window;
        STEP_LOG(L_INFO, "MainWindow created");

        step::gui::GuiController gui_cotroller;
        STEP_LOG(L_INFO, "GuiController created");

        gui_cotroller.show_main_window_slot();

        if (!step_app->is_started())
            step_app->start();

        STEP_LOG(L_INFO, "Application started");

        STEP_LOG(L_INFO, "Trying to exec application");
        exit_code = app.exec();
    }
    catch (const std::exception& ex)
    {
        STEP_LOG(L_CRITICAL, "Unhandled exception: {}", ex.what());
        throw;
    }
    catch (...)
    {
        STEP_LOG(L_CRITICAL, "Unknown unhandled exception!");
        throw;
    }

    return exit_code;
}