import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: view

    property int flashAddr: 0x00000
    property bool updating: false
    property int updatePercent: 0
    property string selectedFilePath: ""
    property string selectedFileName: ""
    property string espPhase: ""

    Connections {
        target: m1device
        function onEspInfoReceived(version) {
            espVersionLabel.text = version
        }
        function onEspUpdateProgress(percent) {
            view.updatePercent = percent
        }
        function onEspUpdateStatus(status) {
            view.espPhase = status
        }
        function onEspUpdateComplete() {
            view.updating = false
            view.espPhase = ""
            statusLabel.text = "Success! ESP32 firmware flashed successfully!\n" +
                               "Reboot the M1 to start the new firmware."
            statusLabel.color = "#4CAF50"
            statusLabel.visible = true
        }
        function onEspUpdateError(message) {
            view.updating = false
            view.espPhase = ""
            statusLabel.text = "Error: " + message
            statusLabel.color = "#F44336"
            statusLabel.visible = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: "ESP32-C6 Firmware Update"
            font.pixelSize: 24
            font.bold: true
        }

        Label {
            text: "Flash ESP32-C6 coprocessor firmware directly via the M1."
            color: Material.hintTextColor
        }

        // ESP32 status
        Pane {
            Layout.fillWidth: true
            Material.elevation: 1

            RowLayout {
                anchors.fill: parent
                spacing: 12

                Label { text: "ESP32 Status:"; font.bold: true }
                Label {
                    text: m1device.esp32Ready ? "Ready" : "Not initialized"
                    color: m1device.esp32Ready ? "#4CAF50" : "#F44336"
                }
                Item { Layout.fillWidth: true }
                Label { text: "Version:" }
                Label {
                    id: espVersionLabel
                    text: m1device.esp32Version.length > 0 ? m1device.esp32Version : "Unknown"
                }
                Button {
                    text: "Initialize"
                    visible: !m1device.esp32Ready
                    enabled: m1device.connected && !view.updating
                    onClicked: m1device.initEsp32()
                }
                Button {
                    text: "Refresh"
                    enabled: m1device.connected
                    onClicked: {
                        m1device.requestDeviceInfo()
                        m1device.requestEspInfo()
                    }
                }
            }
        }

        // Flash address selector
        Pane {
            Layout.fillWidth: true
            Material.elevation: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Label { text: "Flash Configuration"; font.bold: true }

                ButtonGroup { id: imageTypeGroup }

                RadioButton {
                    id: factoryRadio
                    text: "Factory Image — bootloader + partition table + app (0x00000)"
                    checked: true
                    ButtonGroup.group: imageTypeGroup
                    onCheckedChanged: if (checked) view.flashAddr = 0x00000
                }

                RadioButton {
                    id: appRadio
                    text: "App-Only — application partition only (0x60000)"
                    ButtonGroup.group: imageTypeGroup
                    onCheckedChanged: if (checked) view.flashAddr = 0x60000
                }

                Label {
                    text: factoryRadio.checked
                          ? "Factory image includes bootloader + partition table + app. " +
                            "Use for first-time flash or when changing partition layout."
                          : "App-only image is smaller and faster. " +
                            "Use only if bootloader and partition table are already correct."
                    wrapMode: Text.WordWrap
                    font.pixelSize: 11
                    color: Material.hintTextColor
                    Layout.fillWidth: true
                }
            }
        }

        // File selection + flash
        Pane {
            Layout.fillWidth: true
            Material.elevation: 1

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Label { text: "Firmware File"; font.bold: true }

                RowLayout {
                    spacing: 12

                    Button {
                        text: "Select File..."
                        enabled: m1device.connected && !view.updating
                        onClicked: espFileDialog.open()
                    }

                    Label {
                        text: view.selectedFileName.length > 0
                              ? view.selectedFileName
                              : "No file selected"
                        color: view.selectedFileName.length > 0
                               ? Material.foreground
                               : Material.hintTextColor
                        Layout.fillWidth: true
                        elide: Text.ElideMiddle
                    }
                }

                RowLayout {
                    spacing: 12

                    Button {
                        text: "Flash ESP32"
                        highlighted: true
                        enabled: m1device.connected && !view.updating
                                 && view.selectedFilePath.length > 0
                        onClicked: confirmDialog.open()
                    }

                    Label {
                        text: "Target: 0x" + view.flashAddr.toString(16).toUpperCase().padStart(5, "0")
                              + (factoryRadio.checked ? " (Factory)" : " (App-Only)")
                        font.family: "Courier New"
                        font.pixelSize: 12
                        color: Material.hintTextColor
                    }
                }

                // Progress
                ProgressBar {
                    visible: view.updating
                    Layout.fillWidth: true
                    value: view.updatePercent / 100.0
                }

                Label {
                    visible: view.updating
                    text: view.espPhase.length > 0
                          ? view.espPhase + "  " + view.updatePercent + "%"
                          : "Flashing... " + view.updatePercent + "%"
                }
            }
        }

        // Status
        Label {
            id: statusLabel
            visible: false
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Item { Layout.fillHeight: true }
    }

    FileDialog {
        id: espFileDialog
        title: "Select ESP32 Firmware Binary"
        nameFilters: ["Binary files (*.bin)", "All files (*)"]
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            view.selectedFilePath = path
            // Extract filename for display
            var parts = path.split(/[/\\]/)
            view.selectedFileName = parts[parts.length - 1]
            statusLabel.visible = false
        }
    }

    Dialog {
        id: confirmDialog
        title: "Confirm ESP32 Firmware Flash"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            spacing: 12

            Label {
                text: "Flash firmware directly to ESP32?"
                font.bold: true
            }
            Label {
                text: "File: " + view.selectedFileName
            }
            Label {
                text: "Flash address: 0x" + view.flashAddr.toString(16).toUpperCase().padStart(5, "0")
                      + (factoryRadio.checked ? " (Factory Image)" : " (App-Only)")
            }
            Label {
                text: "This will connect to the ESP32 ROM bootloader, erase the\n" +
                      "target region, and write the firmware directly. The ESP32\n" +
                      "will be reset automatically when flashing completes."
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Material.hintTextColor
            }
        }

        onAccepted: {
            view.updating = true
            view.updatePercent = 0
            view.espPhase = ""
            statusLabel.text = ""
            statusLabel.color = Material.foreground
            statusLabel.visible = false
            m1device.startEspUpdate(view.selectedFilePath, view.flashAddr)
        }
    }
}
