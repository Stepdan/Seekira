#include "high_dpi_fix.hpp"

#include <core/log/log.hpp>

#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

namespace step::gui::utils {

void high_dpi_fix()
{
    STEP_LOG(L_WARN, "There is no dpi fix yet, use AA_EnableHighDpiScaling");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    //     if (const char* dpiPolicy = getenv("MOVAVI_DPI_POLICY"))
    //     {
    //         if (std::string(dpiPolicy) == "NOSCALE")
    //             return;
    //     }

    // #ifdef Q_OS_WIN
    //     // On Windows >= 8.1:
    //     // - do not use Qt scaling API
    //     // - tell Windows that we are old legacy software
    //     // So Windows uses scaling from virtual DPI (96) to real for current display
    //     //
    //     // This solution is not perfect because scaled graphics looks ugly
    //     // But it avoids all rendering artifacts and native NAG painting issue (they ignore system DPI)
    //     bool use_qt_scaling = false;

    //     typedef HRESULT(__stdcall * DpiCall)(int);

    //     Movavi::Core::DyLib shCoreLib;
    //     if (shCoreLib.Open("Shcore.dll"))
    //     {
    //         DpiCall setProcessDPIAwareness = (DpiCall)shCoreLib.GetProc("SetProcessDpiAwareness");
    //         if (setProcessDPIAwareness)
    //             setProcessDPIAwareness(0);  // PROCESS_DPI_UNAWARE
    //         else
    //             useQtScaling = true;
    //     }
    //     else
    //         useQtScaling = true;

    //     if (useQtScaling)
    //         QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // #endif
}

}  // namespace step::gui::utils