/*
* Copyright (c) Mercedes-Benz 2014-2024. All rights reserved unless explicitly stated otherwise.
*
* Mercedes-Benz retains its intellectual property and proprietary rights in and to this software
* and related documentation and any modifications thereto. Any use, reproduction, editing, disclosure
* or distribution of this software and related documentation without an express license agreement
* is strictly prohibited. This code contains confidential information.
*/

import QtQuick
import QtQuick.Window

import org.kde.blurng

Window {
    id: root
    objectName: "Main"
    title: "MainUI"

    transientParent: null
    color: "transparent"
    flags: Qt.FramelessWindowHint
    Component.onCompleted: {
        showFullScreen()
    }

    BlurBehind {
        width: xxx.width; height: xxx.height
        anchors.centerIn: xxx
        opacity: 0.0001

        Rectangle {
            // This is a mask
            color: "yellow"
            anchors.fill: parent
            radius: xxx.radius
        }
    }

    Rectangle {
        id: xxx
        color: "white"
        width: 500
        height: 200
        radius: 20

        opacity: 0.2
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
