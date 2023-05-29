import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.4
import QtQuick.Window 2.15
import QtMultimedia 5.15

import step.VideoFrameProvider 1.0

import "common/functions.js" as Functions

ApplicationWindow {
    id: applicationWindow
    flags: Qt.Window
    objectName: cpObjectsConnectorID.QML_MAIN_WINDOW

    width: 1280
    height: availableHeight.windowAvailableHeight
    visible: true
    //color: "transparent"

    signal setQmlWindowInQMainWindowSignal()

    readonly property var availableHeight: getAvailableHeight()

    Component.onCompleted: {
        cpObjectsConnector.register_emitter(cpObjectsConnectorID.SET_QML_IN_MAIN_WINDOW, this, "setQmlWindowInQMainWindowSignal()")

        //cpObjectsConnector.register_receiver(cpObjectsConnectorID.VIDEO_FRAME_UPDATED, this, "onVideoFrameUpdated()")

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

    Button {
        id: start_video_button
        anchors { top: parent.top; right: parent.right; topMargin: 10; rightMargin: 10 }

        onClicked: startVideoButtonClicked()
    }

    // Callback's
    function startVideoButtonClicked() {
        Functions.startVideo();
    }

    Rectangle {
        id: videoOutputRectangle
        width: parent.width - parent.width * 0.1
        height: parent.height
        //color: "#ff0000"

        VideoOutput {
            id: videoOutput
            anchors.fill: parent;
            source: cpVideoFrameProvider;
        }

        Component.onCompleted: {
            //cpObjectsConnector.register_receiver(cpObjectsConnectorID.VIDEO_FRAME_UPDATED, this, "onVideoFrameUpdated()", true)
        }

        function onVideoFrameUpdated() {
            update()
        }
    }
}
