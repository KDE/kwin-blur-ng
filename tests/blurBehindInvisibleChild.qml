/*
    SPDX-FileCopyrightText: 2025 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercedes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QQC

import org.kde.blurng

Window {
    id: root
    width: 200
    height: 200
    flags: Qt.WA_TranslucentBackground
    color: "transparent"
    visibility: Window.FullScreen
    property bool activated: true

    Rectangle {
        id: frame
        anchors {
            fill: parent
            margins: 150
        }
        opacity: .20
        color: "green"
        clip: true
        visible: visibleButton.checked

        BlurBehind {
            id: blur
            width: rect1.width; height: rect1.height
            anchors.centerIn: rect1
            opacity: 0.0001
            activated: root.activated

            Rectangle {
                // This is a mask
                color: "yellow"
                anchors.fill: parent
                radius: rect1.radius
            }
        }

        Rectangle {
            id: rect1
            color: "white"
            width: 500
            height: 200
            radius: 20

            opacity: 0.2
            Drag.active: dragArea1.drag.active
            Drag.hotSpot.x: width / 2
            Drag.hotSpot.y: height / 2

            MouseArea {
                id: dragArea1
                anchors.fill: parent

                drag.target: parent
            }
        }
    }
    ColumnLayout {
        anchors {
            left: frame.left
            right: frame.right
            top: frame.bottom
        }
        Text {
            Layout.alignment: Qt.AlignCenter
            color: "yellow"
            text: frame.visible ? "visible" : "not visible"
        }
        QQC.Slider {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: frame.opacity * 100
            onMoved: frame.opacity = value / 100
        }
        QQC.Slider {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: blur.intensity * 100
            onMoved: blur.intensity = value / 100
        }
        QQC.Button {
            id: visibleButton
            checkable: true
            checked: true
        }
    }
}
