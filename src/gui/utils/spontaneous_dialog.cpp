#include "spontaneous_dialog.hpp"

#include <QApplication>
#include <QPointer>
#include <QWidget>

namespace step::gui::utils::spontaneous {

namespace {
QPointer<QWidget> s_main_window;
}

void set_main_window(QWidget* main_window) { s_main_window = main_window; }

QWidget* get_main_window() { return !s_main_window.isNull() ? s_main_window.data() : QApplication::activeWindow(); }

}  // namespace step::gui::utils::spontaneous