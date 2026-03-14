import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Dialog {
    id: dialog
    title: "Connect to M1"
    modal: true
    anchors.centerIn: parent
    width: 400
    standardButtons: Dialog.Cancel

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Label {
            text: "Select a serial port to connect to your M1 device."
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // Auto-detected M1 devices
        GroupBox {
            title: "Detected M1 Devices"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent

                Label {
                    visible: deviceDiscovery.availablePorts.length === 0
                    text: "No M1 devices detected.\nMake sure your M1 is connected via USB."
                    color: Material.hintTextColor
                    font.italic: true
                }

                Repeater {
                    model: deviceDiscovery.availablePorts
                    delegate: Button {
                        text: "Connect to " + modelData
                        Layout.fillWidth: true
                        highlighted: true
                        onClicked: {
                            m1device.connectToDevice(modelData)
                            dialog.close()
                        }
                    }
                }
            }
        }

        // Manual port entry
        GroupBox {
            title: "Manual Connection"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent

                TextField {
                    id: manualPort
                    placeholderText: "COM port (e.g., COM3)"
                    Layout.fillWidth: true
                }

                Button {
                    text: "Connect"
                    enabled: manualPort.text.length > 0
                    onClicked: {
                        m1device.connectToDevice(manualPort.text)
                        dialog.close()
                    }
                }
            }
        }
    }
}
