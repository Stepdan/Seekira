import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.4
import QtQuick.Window 2.15
import QtMultimedia 5.15

import step.gui.VideoFrameProvider 1.0

import "../common/constants.js" as Constants

Item {
    id: playerFrameView
    width: parent.width - Constants.kPlayerSidePanelWidth
    height: parent.height - Constants.kPlayerControlPanelHeight - Constants.kPlayerTopPanelHeight

    Rectangle {
        anchors.fill: parent;
        color: "#191B23"

        VideoOutput {
            id: playerFrameViewVideoOutput
            anchors.fill: parent;
            source: cpVideoFrameProvider;
        }
    }
}