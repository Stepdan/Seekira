import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.4

ApplicationWindow {
    id: applicationWindow

    objectName: "mainWindow"
    title: "Sightrack App"
    flags: Qt.WindowTitleHint | Qt.WindowMinimizeButtonHint

    FontLoader {
        id: monoFont
        source: "qrc:/fonts/RobotoMono-Medium.ttf"
    }
}
