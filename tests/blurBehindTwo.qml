/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercedes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC2

import org.kde.blurng

Window {
    id: root
    width: 200
    height: 200
    flags: Qt.WA_TranslucentBackground
    color: "transparent"
    visibility: Window.FullScreen
    property bool activated: true

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.visible = false
            aux.show()
        }
    }

    Window {
        id: aux
        width: 200
        height: 200
        visibility: root.visibility === Window.Hidden ? Window.Windowed : Window.Hidden
        Rectangle {
            color: "red"
            anchors.fill: parent
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.visible = true;
            }
        }
        Text {
            anchors.centerIn: parent
            text: "Restore!"
        }
    }

    BlurBehind {
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

    BlurBehind {
        id: bb
        width: rect2.width; height: rect2.height
        anchors.centerIn: rect2
        opacity: 0.0001
        activated: root.activated

        Rectangle {
            // This is a mask
            color: "yellow"
            anchors.fill: parent
            radius: rect2.radius
        }
    }

    Rectangle {
        id: rect2
        color: "white"
        x: parent.width / 2
        y: parent.height / 2
        width: 500
        height: 200
        radius: 50

        opacity: 0.2
        Drag.active: dragArea2.drag.active
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2

        MouseArea {
            id: dragArea2
            anchors.fill: parent

            drag.target: parent
            onDoubleClicked: {
                bb.activated = !bb.activated
            }
        }
    }
}
