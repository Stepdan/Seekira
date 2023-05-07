import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.4

ApplicationWindow {
    id: window

    objectName: "mainWindow"
    title: "StepKit App"
    flags: Qt.WindowTitleHint | Qt.WindowMinimizeButtonHint
    Material.theme: Settings.styleMaterialTheme()
    Material.accent: Settings.styleMaterialAccent()

    FontLoader {
        id: monoFont
        source: "qrc:/fonts/RobotoMono-Medium.ttf"
    }
}
