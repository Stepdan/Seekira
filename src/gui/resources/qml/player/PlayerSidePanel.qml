import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.4
import QtQuick.Layouts 1.4
import QtQuick.Window 2.15

import "../common/constants.js" as Constants

Rectangle {
    id: playerSidePanel
    width: Constants.kPlayerSidePanelWidth
    height: parent.height
    color: Constants.kClrMain
}