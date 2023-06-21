import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.4
import QtQuick.Window 2.15

import "../common/constants.js" as Constants

Rectangle {
    id: playerTopPanel
    width: parent.width
    height: Constants.kPlayerTopPanelHeight
    color: "#222532"

    Image {
        id: logo
        source: "qrc:/icons/logo.png"
        anchors { top: parent.top; left: parent.left; topMargin: 10; leftMargin: 5 }
    }
}