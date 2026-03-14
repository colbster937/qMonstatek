import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

/*
 * VirtualButtonPad — 6-button D-pad matching M1 physical buttons.
 * Layout:
 *              [  UP  ]
 *   [LEFT]     [  OK  ]     [RIGHT]
 *              [ DOWN ]
 *                              [BACK]
 */
Item {
    id: pad
    implicitWidth: 280
    implicitHeight: 180

    property bool enabled: m1device.connected

    // D-pad center position
    readonly property real cx: width * 0.4
    readonly property real cy: height * 0.5
    readonly property real btnSize: 52

    // UP button
    RoundButton {
        x: cx - btnSize/2
        y: cy - btnSize - 8
        width: btnSize; height: btnSize
        text: "▲"
        font.pixelSize: 18
        enabled: pad.enabled
        Material.background: Material.BlueGrey
        onClicked: m1device.buttonClick(1)
    }

    // DOWN button
    RoundButton {
        x: cx - btnSize/2
        y: cy + 8
        width: btnSize; height: btnSize
        text: "▼"
        font.pixelSize: 18
        enabled: pad.enabled
        Material.background: Material.BlueGrey
        onClicked: m1device.buttonClick(4)
    }

    // LEFT button
    RoundButton {
        x: cx - btnSize - 8
        y: cy - btnSize/2
        width: btnSize; height: btnSize
        text: "◄"
        font.pixelSize: 18
        enabled: pad.enabled
        Material.background: Material.BlueGrey
        onClicked: m1device.buttonClick(2)
    }

    // RIGHT button
    RoundButton {
        x: cx + 8
        y: cy - btnSize/2
        width: btnSize; height: btnSize
        text: "►"
        font.pixelSize: 18
        enabled: pad.enabled
        Material.background: Material.BlueGrey
        onClicked: m1device.buttonClick(3)
    }

    // OK button (center)
    RoundButton {
        x: cx - btnSize * 0.4
        y: cy - btnSize * 0.4
        width: btnSize * 0.8; height: btnSize * 0.8
        text: "OK"
        font.pixelSize: 12
        font.bold: true
        enabled: pad.enabled
        Material.background: Material.Green
        onClicked: m1device.buttonClick(0)
    }

    // BACK button (separate, to the right)
    RoundButton {
        x: cx + btnSize + 32
        y: cy - 20
        width: 64; height: 40
        text: "BACK"
        font.pixelSize: 10
        enabled: pad.enabled
        Material.background: Material.Red
        onClicked: m1device.buttonClick(5)
    }

    // Keyboard hint
    Label {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: "Arrow keys = D-pad  |  Enter = OK  |  Esc = Back"
        font.pixelSize: 10
        color: Material.hintTextColor
    }
}
