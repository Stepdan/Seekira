import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Window 2.15

Window {
    id: playerWindow
    flags: Qt.Window
    objectName: cpObjectsConnectorID.QML_PLAYER_WINDOW

    width: 1280
    height: 720
    visible: true

    PlayerTopPanel {
        id: playerTopPanel
        anchors { top: parent.top; left: parent.left; }
    }

    PlayerControlPanel {
        id: playerControlPanel
        anchors { bottom: parent.bottom; left: parent.left; }
    }

    PlayerSidePanel {
        id: playerSidePanel
        anchors { top: parent.top; right: parent.right; }
    }

    PlayerFrameView {
        id: playerFrameView
        anchors { top: playerTopPanel.bottom; left: parent.left; }
    }
}
