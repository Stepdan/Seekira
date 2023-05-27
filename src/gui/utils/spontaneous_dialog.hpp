#pragma once

class QWidget;

namespace step::gui::utils::spontaneous {

/// @brief выставление главного окна для приложения
void set_main_window(QWidget* main_window);

/// @brief получение главного окна
QWidget* get_main_window();

}  // namespace step::gui::utils::spontaneous