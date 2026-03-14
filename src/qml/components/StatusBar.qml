import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ToolBar {
    Material.elevation: 2

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12

        // Connection indicator
        Rectangle {
            width: 10; height: 10
            radius: 5
            color: m1device.connected ? "#4CAF50" : "#F44336"
        }

        Label {
            text: m1device.connected ? m1device.firmwareVersion : "No device"
            font.pixelSize: 13
        }

        Item { Layout.fillWidth: true }

        // Battery indicator
        RowLayout {
            visible: m1device.connected
            spacing: 4

            Label {
                text: m1device.batteryCharging ? "⚡" : "🔋"
                font.pixelSize: 14
            }
            Label {
                text: m1device.batteryLevel + "%"
                font.pixelSize: 12
            }
        }

        // SD card indicator
        Label {
            visible: m1device.connected && m1device.sdCardPresent
            text: "💾 SD"
            font.pixelSize: 12
            Layout.leftMargin: 12
        }

        // ESP32 indicator
        Label {
            visible: m1device.connected
            text: m1device.esp32Ready ? "📡 ESP OK" : "📡 ESP ✗"
            font.pixelSize: 12
            color: m1device.esp32Ready ? Material.foreground : "#F44336"
            Layout.leftMargin: 12
        }

        // Connect/Disconnect button
        ToolButton {
            text: m1device.connected ? "Disconnect" : "Connect"
            onClicked: {
                if (m1device.connected) {
                    m1device.disconnect()
                } else {
                    deviceSelector.open()
                }
            }
        }
    }
}
