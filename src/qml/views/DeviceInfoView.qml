import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ScrollView {
    id: view

    ColumnLayout {
        width: view.width
        spacing: 16
        anchors.margins: 24

        Label {
            text: "Device Information"
            font.pixelSize: 24
            font.bold: true
            Layout.topMargin: 24
            Layout.leftMargin: 24
        }

        // Not connected message
        Label {
            visible: !m1device.connected
            text: "Connect your M1 device to view information."
            font.pixelSize: 14
            color: Material.hintTextColor
            Layout.leftMargin: 24
        }

        // Connected but no RPC data
        Label {
            visible: m1device.connected && !m1device.hasDeviceInfo
            text: "Device connected but firmware does not support device info.\n" +
                  "Use the DFU Flash page to install compatible firmware."
            font.pixelSize: 12
            color: "#FF9800"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.leftMargin: 24
            Layout.rightMargin: 24
        }

        // Info cards grid
        GridLayout {
            visible: m1device.connected
            columns: 2
            columnSpacing: 16
            rowSpacing: 16
            Layout.leftMargin: 24
            Layout.rightMargin: 24

            // Firmware Version card
            Pane {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                Material.elevation: 2
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "Firmware Version"; font.bold: true; color: Material.accent }
                    Label {
                        text: m1device.hasDeviceInfo ? m1device.firmwareVersion : "No data"
                        font.pixelSize: 20
                        color: m1device.hasDeviceInfo ? Material.foreground : Material.hintTextColor
                    }
                }
            }

            // Active Bank card
            Pane {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                Material.elevation: 2
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "Active Boot Bank"; font.bold: true; color: Material.accent }
                    Label {
                        text: m1device.hasDeviceInfo ? "Bank " + m1device.activeBank : "No data"
                        font.pixelSize: 20
                        color: m1device.hasDeviceInfo ? Material.foreground : Material.hintTextColor
                    }
                }
            }

            // Battery card
            Pane {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                Material.elevation: 2
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4
                    Label { text: "Battery"; font.bold: true; color: Material.accent }
                    RowLayout {
                        Label {
                            text: m1device.batteryLevel + "%"
                            font.pixelSize: 20
                        }
                        Label {
                            text: {
                                var s = m1device.chargeState
                                if (s === 3) return "Charge Complete"
                                if (s === 2) return "Fast Charging"
                                if (s === 1) return "Pre-Charging"
                                return ""
                            }
                            font.pixelSize: 12
                            color: "#4CAF50"
                            visible: m1device.chargeState > 0
                        }
                    }
                    ProgressBar {
                        Layout.fillWidth: true
                        value: m1device.batteryLevel / 100.0
                        Material.accent: m1device.batteryLevel > 20 ? Material.Green : Material.Red
                    }
                    GridLayout {
                        columns: 2
                        columnSpacing: 16
                        rowSpacing: 2
                        Label { text: "Voltage:"; font.pixelSize: 11; color: Material.hintTextColor }
                        Label { text: (m1device.batteryVoltage / 1000.0).toFixed(2) + " V"; font.pixelSize: 11 }
                        Label { text: "Current:"; font.pixelSize: 11; color: Material.hintTextColor }
                        Label { text: m1device.batteryCurrent + " mA"; font.pixelSize: 11 }
                        Label { text: "Temp:"; font.pixelSize: 11; color: Material.hintTextColor }
                        Label { text: m1device.batteryTemp + " \u00B0C"; font.pixelSize: 11 }
                        Label { text: "Health:"; font.pixelSize: 11; color: Material.hintTextColor }
                        Label {
                            text: m1device.batteryHealth + "%"
                            font.pixelSize: 11
                            color: m1device.batteryHealth > 70 ? Material.foreground : "#F44336"
                        }
                    }
                }
            }

            // SD Card card
            Pane {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                Material.elevation: 2
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "SD Card"; font.bold: true; color: Material.accent }
                    Label {
                        text: m1device.sdCardPresent ? m1device.sdCapacity : "Not inserted"
                        font.pixelSize: 14
                    }
                }
            }

            // ESP32 card
            Pane {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                Material.elevation: 2
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "ESP32-C6 Coprocessor"; font.bold: true; color: Material.accent }
                    Label {
                        text: m1device.esp32Ready
                              ? "Ready — " + m1device.esp32Version
                              : "Not initialized"
                        font.pixelSize: 14
                        color: m1device.esp32Ready ? Material.foreground : "#F44336"
                    }
                }
            }

            // Connection card
            Pane {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                Material.elevation: 2
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "Connection"; font.bold: true; color: Material.accent }
                    Label { text: "Port: " + m1device.portName; font.pixelSize: 14 }
                }
            }
        }

        // Refresh button
        Button {
            visible: m1device.connected
            text: "Refresh"
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 8
            onClicked: m1device.requestDeviceInfo()
        }
    }
}
