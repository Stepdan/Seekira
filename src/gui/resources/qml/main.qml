import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.4
import QtQuick.Window 2.15
import QtMultimedia 5.15

import step.gui.VideoFrameProvider 1.0

import "player"

import "common/functions.js" as Functions

ApplicationWindow {
    id: applicationWindow
    flags: Qt.Window
    objectName: cpObjectsConnectorID.QML_MAIN_WINDOW

    width: 640
    height: 480
    visible: true
    //color: "transparent"

    signal setQmlWindowInQMainWindowSignal()

    readonly property var availableHeight: getAvailableHeight()

    Component.onCompleted: {
        cpObjectsConnector.register_emitter(cpObjectsConnectorID.SET_QML_IN_MAIN_WINDOW, this, "setQmlWindowInQMainWindowSignal()")

        setQmlWindowInQMainWindowSignal()
    }

    onClosing: {
        close.acceped = Functions.mainWindowClosing()
    }

    function getAvailableHeight() {
        var outputData = {
            windowAvailableHeight: 724,
            desktopAvailableHeight: Screen.height - 40,
            heightOffsetForMainWindow: 0
        }

        if (outputData.desktopAvailableHeight < outputData.windowAvailableHeight) {
            outputData.heightOffsetForMainWindow = 44
            outputData.windowAvailableHeight -= outputData.heightOffsetForMainWindow
            outputData.desktopAvailableHeight = outputData.windowAvailableHeight
        }

        return outputData
    }

    //---------------Player---------------

    Loader {
        id: playerWindowLoader
    }

    Button {
        id: playerButton
        anchors { top: parent.top; right: parent.right; topMargin: 10; rightMargin: 10 }
        text: "Player"

        onClicked: playerButtonClicked()
    }

    // Callback's
    function playerButtonClicked() {
        playerWindowLoader.source = "player/PlayerWindow.qml"
        playerWindowLoader.item.show()
        cpPlayerController.open_file("")
    }
}
