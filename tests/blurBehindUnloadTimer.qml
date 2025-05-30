/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercedes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls as QQC2

import org.kde.blurng

Window {
    id: root
    width: 200
    height: 200
    flags: Qt.WA_TranslucentBackground
    color: "transparent"
    visibility: timerVisible ? Window.FullScreen : Window.Hidden
    property bool timerActivated: true
    property bool timerVisible: true
    property bool timerLoaderActive: true

    Timer {
        interval: 500
        running: true
        repeat: true
        onTriggered: {
            root.timerVisible = !root.timerVisible
        }
    }

    Timer {
        interval: 700
        running: true
        repeat: true
        onTriggered: {
            root.timerLoaderActive = !root.timerLoaderActive
        }
    }

    Timer {
        interval: 900
        running: true
        repeat: true
        onTriggered: {
            root.timerActivated = !root.timerActivated
        }
    }

    Loader {
        id: loader1
        anchors.centerIn: rect1
        width: rect1.width
        height: rect1.height
        active: root.timerLoaderActive

        sourceComponent: BlurBehind {
            opacity: 0.0001
            activated: root.timerActivated

            Rectangle {
                // This is a mask
                color: "yellow"
                anchors.fill: parent
                radius: rect1.radius
            }
        }
    }

    Rectangle {
        id: rect1
        color: "white"
        width: 500
        height: 200
        radius: 20

        opacity: 0.2

        MouseArea {
            anchors.fill: parent
            onClicked: Qt.quit()
        }
    }

    BlurBehind {
        id: blur2
        x: 700
        y: 50
        opacity: 0.0001
        activated: true
        property bool big: true
        width: big ? 600 : 200
        height: big ? 600 : 200

        Behavior on width {
            NumberAnimation { duration: 1000 }
        }
        Behavior on height {
            NumberAnimation { duration: 1000 }
        }

        Timer {
            interval: 1500
            running: true
            repeat: true
            onTriggered: {
                parent.big = !parent.big
            }
        }

        Rectangle {
            // This is a mask
            color: "yellow"
            anchors.fill: parent
            radius: rect1.radius
        }
    }

    Rectangle {
        color: "red"
        anchors.fill: blur2
        radius: rect1.radius

        opacity: 0.2

        MouseArea {
            anchors.fill: parent
            onClicked: Qt.quit()
        }
    }
}
