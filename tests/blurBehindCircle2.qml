/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercedes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC2

import org.kde.blurng as BNG

Window {
    id: main
    width: 200; height: 200
    flags: Qt.WA_TranslucentBackground
    color: "transparent"
    Component.onCompleted: {
        showFullScreen()
    }

    BNG.BlurBehind {
        id: ppp
        x: 10; y: 10
        width: 50; height: 50
        // activated: false
        opacity: .7

        Rectangle {
            color: "red"
            radius: width
            anchors.fill: parent
        }

        Timer {
            property real angle: 0
            property int centerX: ppp.x + main.width / 2
            property int centerY: ppp.y + main.height / 2
            property int radius: Math.min(main.width, main.height) / 2

            interval: 1000/60 // 60fps
            repeat: true
            running: true
            onTriggered: {
                angle += 0.01
                if (angle > Math.PI * 2) {
                    angle -= Math.PI * 2
                }
                ppp.x = centerX + radius * Math.cos(angle) - ppp.width / 2;
                ppp.y = centerY + radius * Math.sin(angle) - ppp.height / 2;
            }
        }
    }
}
