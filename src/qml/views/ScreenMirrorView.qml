import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "../components"

Item {
    id: view
    focus: true

    // Keyboard shortcuts for remote control
    Keys.onUpPressed:     if (m1device.connected) m1device.buttonClick(1)
    Keys.onDownPressed:   if (m1device.connected) m1device.buttonClick(4)
    Keys.onLeftPressed:   if (m1device.connected) m1device.buttonClick(2)
    Keys.onRightPressed:  if (m1device.connected) m1device.buttonClick(3)
    Keys.onReturnPressed: if (m1device.connected) m1device.buttonClick(0)
    Keys.onEscapePressed: if (m1device.connected) m1device.buttonClick(5)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Title row
        RowLayout {
            Label {
                text: "Screen Mirror"
                font.pixelSize: 24
                font.bold: true
                Layout.fillWidth: true
            }

            // FPS selector
            Label { text: "FPS:" }
            SpinBox {
                id: fpsSpinner
                from: 1; to: 15; value: 10
                editable: true
            }

            // Stream toggle
            Button {
                text: m1device.screenStreaming ? "Stop Stream" : "Start Stream"
                enabled: m1device.connected
                highlighted: m1device.screenStreaming
                onClicked: {
                    if (m1device.screenStreaming) {
                        m1device.stopScreenStream()
                    } else {
                        m1device.startScreenStream(fpsSpinner.value)
                    }
                }
            }

            // Screenshot
            Button {
                text: "Screenshot"
                enabled: m1device.connected
                onClicked: {
                    var path = "screenshot_" + Date.now() + ".png"
                    if (m1device.saveScreenshot(path)) {
                        screenshotLabel.text = "Saved: " + path
                        screenshotLabel.visible = true
                    }
                }
            }
        }

        // Screen display (centered)
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            MonoDisplay {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 8
                width: 512
                height: 256
            }
        }

        // Virtual button pad
        VirtualButtonPad {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 180
            Layout.preferredWidth: 280
        }

        // Screenshot notification
        Label {
            id: screenshotLabel
            visible: false
            color: Material.accent
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter

            Timer {
                running: screenshotLabel.visible
                interval: 3000
                onTriggered: screenshotLabel.visible = false
            }
        }
    }
}
