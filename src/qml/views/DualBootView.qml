import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: view

    property var fwInfo: null
    property string swapStatus: ""
    property string swapStatusColor: Material.foreground

    Connections {
        target: m1device
        function onFwInfoReceived(info) {
            view.fwInfo = info
        }
        function onBankSwapStarted() {
            view.swapStatus = "Success! Bank swap accepted — device is rebooting..."
            view.swapStatusColor = "#4CAF50"
        }
        function onBankSwapError(message) {
            view.swapStatus = "Bank swap failed: " + message
            view.swapStatusColor = "#F44336"
        }
        function onConnectionChanged() {
            if (m1device.connected) {
                view.fwInfo = null
                view.swapStatus = ""
                m1device.requestFwInfo()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        Label {
            text: "Dual Boot Manager"
            font.pixelSize: 24
            font.bold: true
        }

        Label {
            text: "Your M1 has dual-bank flash: two independent firmware slots. " +
                  "One bank is active (booting), the other is a backup. " +
                  "When you update firmware, the new version is written to the inactive bank. " +
                  "You can swap banks to activate the new version, or swap back to rollback."
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            font.pixelSize: 12
            color: Material.hintTextColor
        }

        Label {
            visible: !m1device.connected
            text: "Connect your M1 to manage boot banks."
            color: Material.hintTextColor
        }

        // Bank cards side by side
        RowLayout {
            visible: m1device.connected
            Layout.fillWidth: true
            spacing: 16

            // Bank 1
            Pane {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                Material.elevation: m1device.activeBank === 1 ? 4 : 1

                Rectangle {
                    anchors.fill: parent
                    color: m1device.activeBank === 1 ? "#1B5E20" : "transparent"
                    opacity: 0.15
                    radius: 4
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    RowLayout {
                        Label {
                            text: "Bank 1"
                            font.pixelSize: 20
                            font.bold: true
                        }
                        Label {
                            text: m1device.activeBank === 1 ? " ● ACTIVE" : ""
                            color: "#4CAF50"
                            font.bold: true
                        }
                    }

                    Label {
                        text: {
                            if (!fwInfo || !fwInfo.bank1) return "Loading..."
                            var v = "v" + fwInfo.bank1.major + "." +
                                    fwInfo.bank1.minor + "." +
                                    fwInfo.bank1.build + "." +
                                    fwInfo.bank1.rc
                            if (fwInfo.bank1.c3Revision > 0)
                                v += "-C3." + fwInfo.bank1.c3Revision
                            return v
                        }
                        font.pixelSize: 16
                    }

                    Label {
                        text: fwInfo && fwInfo.bank1
                              ? (fwInfo.bank1.crcValid ? "CRC: Valid" : "CRC: Invalid")
                              : ""
                        color: (fwInfo && fwInfo.bank1 && fwInfo.bank1.crcValid)
                               ? "#4CAF50" : "#F44336"
                    }

                    Label {
                        text: fwInfo && fwInfo.bank1 && fwInfo.bank1.imageSize > 0
                              ? "Size: " + (fwInfo.bank1.imageSize / 1024).toFixed(0) + " KB"
                              : "Size: Unknown"
                        font.pixelSize: 11
                        color: Material.hintTextColor
                    }

                    Label {
                        visible: fwInfo && fwInfo.bank1 && fwInfo.bank1.buildDate.length > 0
                        text: "Built: " + (fwInfo && fwInfo.bank1 ? fwInfo.bank1.buildDate : "")
                        font.pixelSize: 11
                        color: Material.hintTextColor
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // Swap arrow
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 8

                Label {
                    text: "⇄"
                    font.pixelSize: 32
                    Layout.alignment: Qt.AlignHCenter
                }

                Button {
                    id: swapBtn
                    text: inactiveBankValid ? "Swap Banks" : "Swap Banks (No FW)"
                    enabled: m1device.connected && inactiveBankValid
                    highlighted: true
                    Layout.alignment: Qt.AlignHCenter
                    onClicked: swapConfirm.open()

                    property bool inactiveBankValid: {
                        if (!fwInfo) return false
                        var inactive = (m1device.activeBank === 1) ? fwInfo.bank2 : fwInfo.bank1
                        if (!inactive) return false
                        // Check if bank has any firmware (non-zero version or image size)
                        var hasFirmware = inactive.major > 0 || inactive.minor > 0 ||
                                          inactive.build > 0 || inactive.imageSize > 0
                        if (!hasFirmware) return false
                        // If CRC extension present (imageSize > 0), require CRC valid
                        if (inactive.imageSize > 0 && !inactive.crcValid) return false
                        return true
                    }

                    ToolTip.visible: hovered && !inactiveBankValid && m1device.connected
                    ToolTip.text: "The inactive bank has no valid firmware. Upload firmware first."
                }
            }

            // Bank 2
            Pane {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                Material.elevation: m1device.activeBank === 2 ? 4 : 1

                Rectangle {
                    anchors.fill: parent
                    color: m1device.activeBank === 2 ? "#1B5E20" : "transparent"
                    opacity: 0.15
                    radius: 4
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    RowLayout {
                        Label {
                            text: "Bank 2"
                            font.pixelSize: 20
                            font.bold: true
                        }
                        Label {
                            text: m1device.activeBank === 2 ? " ● ACTIVE" : ""
                            color: "#4CAF50"
                            font.bold: true
                        }
                    }

                    Label {
                        text: {
                            if (!fwInfo || !fwInfo.bank2) return "Loading..."
                            var v = "v" + fwInfo.bank2.major + "." +
                                    fwInfo.bank2.minor + "." +
                                    fwInfo.bank2.build + "." +
                                    fwInfo.bank2.rc
                            if (fwInfo.bank2.c3Revision > 0)
                                v += "-C3." + fwInfo.bank2.c3Revision
                            return v
                        }
                        font.pixelSize: 16
                    }

                    Label {
                        text: fwInfo && fwInfo.bank2
                              ? (fwInfo.bank2.crcValid ? "CRC: Valid" : "CRC: Invalid")
                              : ""
                        color: (fwInfo && fwInfo.bank2 && fwInfo.bank2.crcValid)
                               ? "#4CAF50" : "#F44336"
                    }

                    Label {
                        text: fwInfo && fwInfo.bank2 && fwInfo.bank2.imageSize > 0
                              ? "Size: " + (fwInfo.bank2.imageSize / 1024).toFixed(0) + " KB"
                              : "Size: Unknown"
                        font.pixelSize: 11
                        color: Material.hintTextColor
                    }

                    Label {
                        visible: fwInfo && fwInfo.bank2 && fwInfo.bank2.buildDate.length > 0
                        text: "Built: " + (fwInfo && fwInfo.bank2 ? fwInfo.bank2.buildDate : "")
                        font.pixelSize: 11
                        color: Material.hintTextColor
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // Refresh button
        Button {
            visible: m1device.connected
            text: "Refresh Bank Info"
            Layout.alignment: Qt.AlignHCenter
            onClicked: m1device.requestFwInfo()
        }

        Label {
            visible: view.swapStatus.length > 0
            text: view.swapStatus
            color: view.swapStatusColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.leftMargin: 24
            font.pixelSize: 14
        }

        // Explanation
        Pane {
            Layout.fillWidth: true
            Material.elevation: 1

            Label {
                width: parent.width
                text: "How it works:\n" +
                      "• Firmware updates write to the INACTIVE bank\n" +
                      "• 'Swap Banks' switches which bank boots next and reboots\n" +
                      "• If something goes wrong, swap back to restore the previous firmware\n" +
                      "• Both banks are independent — swapping does not erase anything"
                wrapMode: Text.WordWrap
                font.pixelSize: 12
                color: Material.hintTextColor
            }
        }
    }

    Dialog {
        id: swapConfirm
        title: "Swap Boot Banks"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            text: "This will swap the active bank and reboot your M1.\n\n" +
                  "Current: Bank " + m1device.activeBank + "\n" +
                  "After swap: Bank " + (m1device.activeBank === 1 ? 2 : 1) + "\n\n" +
                  "The device will disconnect briefly during reboot."
            wrapMode: Text.WordWrap
        }

        onAccepted: m1device.swapBanks()
    }

    Component.onCompleted: {
        if (m1device.connected) m1device.requestFwInfo()
    }
}
