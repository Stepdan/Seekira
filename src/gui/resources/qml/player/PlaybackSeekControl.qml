import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.4
import QtMultimedia 5.15

import step.gui.enum 1.0

import "../common/constants.js" as Constants

Item {
    id: playbackSeekControl

    Rectangle {
        id: playbackSeekControlRect
        color: Constants.kClrMain
        width: parent.width
        height: parent.height
        anchors { top: parent.top; left: parent.left; }

        Text {
            id: mediaTime
            anchors { top: parent.top; topMargin: 5; left: parent.left; leftMargin: 5; }
            font.pixelSize: 10
            width: 100
            text: "00:00:00.000"
            color: "white"
        }

        Slider {
            id: mediaSlider
            width: parent.width - 100
            height: parent.height
            anchors { top: parent.top; right: parent.right; }
            enabled: true
            to: 1.0
            value: 0.0
            snapMode: Slider.NoSnap

            //#4D5BA4

            handle: Rectangle {
                id: mediaSliderHandle
                x: mediaSlider.visualPosition * (mediaSlider.width - width)
                y: (mediaSlider.height - height) / 2
                width: 10
                height: 10
                radius: 10
                color: "white"
            }

            background: Rectangle {
                y: (mediaSlider.height - height) / 2
                height: 4
                radius: 2
                color: Constants.kClrMainSecondary

                Rectangle {
                    width: mediaSlider.visualPosition * parent.width
                    height: parent.height
                    color: Constants.kClrLogoSecondary
                    radius: 2
                }
            }

            onMoved: cpPlayerController.playback_set_position_slot(value)

            Component.onCompleted: {
                cpObjectsConnector.register_receiver(cpObjectsConnectorID.PLAYBACK_POS_UPDATED, this, "on_playback_pos_updated()")
            }

            function on_playback_pos_updated()
            {
                var pos = cpPlayerController.get_position()
                mediaSlider.value = pos / cpPlayerController.get_duration()

                var h = Math.floor(pos / 3600000000)
                var m = Math.floor(pos / 60000000 - h * 60)
                var s = Math.floor(pos / 1000000 - m * 60)
                var ms = Math.floor(pos / 1000 - s * 1000)

                var ms_all = pos / 1000
                var endPos = ~(0 * !!ms_all)  // to trim "Z" or ".sssZ"
                mediaTime.text = new Date(ms_all).toISOString().slice(11, endPos)
            }
        }
    }
}