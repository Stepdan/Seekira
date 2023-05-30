#include "enums.hpp"

#include <QtQml/qqml.h>

namespace step::gui {

void register_qml_enums() { qmlRegisterType<step::gui::Enums>("step.gui.enum", 1, 0, "Enums"); }

}  // namespace step::gui