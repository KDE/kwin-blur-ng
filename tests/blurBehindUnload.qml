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
    visibility: visibleButton.checked ? Window.FullScreen : Window.Hidden

    Window {
        id: aux
        width: 200
        height: 300
        visibility: Window.Windowed
        ColumnLayout {
            QQC2.Button {
                id: visibleButton
                checkable: true
                checked: true
                text: "Toggle visible"
                Shortcut {
                    sequence: "V"
                    context: Qt.ApplicationShortcut
                    onActivated: visibleButton.toggle()
                }
            }
            QQC2.Button {
                id: loaderButton
                checkable: true
                checked: true
                text: "Toggle Loader.active"
                Shortcut {
                    sequence: "L"
                    context: Qt.ApplicationShortcut
                    onActivated: loaderButton.toggle()
                }
            }
            QQC2.Button {
                id: activatedButton
                checkable: true
                checked: true
                text: "Toggle BlurBehind.activated"
                Shortcut {
                    sequence: "B"
                    context: Qt.ApplicationShortcut
                    onActivated: activatedButton.toggle()
                }
            }
        }
    }

    Loader {
        id: loader1
        anchors.centerIn: rect1
        width: rect1.width
        height: rect1.height
        active: loaderButton.checked

        sourceComponent: BlurBehind {
            opacity: 0.0001
            activated: activatedButton.checked

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
    }
}
