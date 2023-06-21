import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.4
import QtQuick.Window 2.15
import QtMultimedia 5.15

import step.gui.enum 1.0

import "../common/constants.js" as Constants

Rectangle {
    id: playerControlPanel
    width: parent.width - Constants.kPlayerSidePanelWidth
    height: Constants.kPlayerControlPanelHeight
    //color: "#313131"
    color: "#222532"

    Button {
        id: playButton
        width: Constants.kPlayerPlayBtnSize
        height: Constants.kPlayerPlayBtnSize
        anchors { top: parent.top; left: parent.left; topMargin: (parent.height-height)/2; leftMargin: (parent.width-width)/2 }

        onClicked: playBtnClicked()
    }

    function playBtnClicked() {
        cpPlayerController.play_state_switch()
    }

    Button {
        id: stepLeftButton
        width: Constants.kPlayerBtnSize
        height: Constants.kPlayerBtnSize
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: (parent.height-Constants.kPlayerPlayBtnSize)/2 + Constants.kPlayerBtnSpace
        anchors.leftMargin: (parent.width-Constants.kPlayerPlayBtnSize)/2 - Constants.kPlayerBtnSize - Constants.kPlayerBtnSpace

        onClicked: stepLeftBtnClicked()
    }

    function stepLeftBtnClicked() {
        cpPlayerController.step_rewind(Enums.PLAYER_DIRECTION_BACKWARD);
    }

    Button {
        id: backwardButton
        width: Constants.kPlayerBtnSize
        height: Constants.kPlayerBtnSize
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: (parent.height-Constants.kPlayerPlayBtnSize)/2 + Constants.kPlayerBtnSpace
        anchors.leftMargin: (parent.width-Constants.kPlayerPlayBtnSize)/2 - 2*Constants.kPlayerBtnSize - 2*Constants.kPlayerBtnSpace

        onClicked: backwardBtnClicked()
    }

    function backwardBtnClicked() {
        cpPlayerController.step_frame(Enums.PLAYER_DIRECTION_BACKWARD);
    }

    Button {
        id: stepRightButton
        width: Constants.kPlayerBtnSize
        height: Constants.kPlayerBtnSize
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: (parent.height-Constants.kPlayerPlayBtnSize)/2 + Constants.kPlayerBtnSpace
        anchors.leftMargin: (parent.width-Constants.kPlayerPlayBtnSize)/2 + Constants.kPlayerPlayBtnSize + Constants.kPlayerBtnSpace

        onClicked: stepRightBtnClicked()
    }

    function stepRightBtnClicked() {
        cpPlayerController.step_rewind(Enums.PLAYER_DIRECTION_FORWARD);
    }

    Button {
        id: forwardButton
        width: Constants.kPlayerBtnSize
        height: Constants.kPlayerBtnSize
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: (parent.height-Constants.kPlayerPlayBtnSize)/2 + Constants.kPlayerBtnSpace
        anchors.leftMargin: (parent.width-Constants.kPlayerPlayBtnSize)/2 + Constants.kPlayerPlayBtnSize + Constants.kPlayerBtnSize + 2*Constants.kPlayerBtnSpace

        onClicked: forwardBtnClicked()
    }

    function forwardBtnClicked() {
        cpPlayerController.step_frame(Enums.PLAYER_DIRECTION_FORWARD);
    }
}