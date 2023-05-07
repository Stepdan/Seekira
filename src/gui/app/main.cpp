#include <log/log.hpp>

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
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        //QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
        QCoreApplication::setApplicationName("StepKit");
        QCoreApplication::setApplicationVersion("1.0.0");
        QApplication app(argc, argv);

        std::shared_ptr<step::gui::MainWindow> main_window = std::make_shared<step::gui::MainWindow>();
        if (!main_window->load())
            return exit_code;

        main_window->show();

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