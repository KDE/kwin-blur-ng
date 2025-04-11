/*
    SPDX-FileCopyrightText: 2023 Aleix Pol Gonzalez <aleix.pol_gonzalez@mercedes-benz.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

import QtQuick
import QtQuick.Window
import QtQuick.Controls as QQC2

import org.kde.blurng as BNG

Window {
    width: 200; height: 200
    flags: Qt.WA_TranslucentBackground
    color: "transparent"
    Component.onCompleted: {
        showFullScreen()
    }

    BNG.BlurBehind {
        x: 10; y: 10
        width: 100; height: 100

        Text {
            text: "x"
            color: "yellow"
            anchors.centerIn: parent
        }

        Drag.active: dragArea.drag.active
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2

        MouseArea {
            id: dragArea
            anchors.fill: parent

            drag.target: parent
        }
    }
}
