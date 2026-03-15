import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: view

    property var fwInfo: null
    property string swapStatus: ""
    property string swapStatusColor: Material.foreground
    property bool eraseInProgress: false

    function isBankEmpty(bank) {
        if (!bank) return true
        // Wiped flash reads as 0xFF (255 per byte)
        if (bank.major === 255 && bank.minor === 255 &&
            bank.build === 255 && bank.rc === 255) return true
        // No version and no image
        if (bank.major === 0 && bank.minor === 0 &&
            bank.build === 0 && bank.rc === 0 &&
            bank.imageSize === 0) return true
        return false
    }

    function bankVersionText(bank) {
        if (!bank) return "Loading..."
        if (isBankEmpty(bank)) return "Empty"
        var v = "v" + bank.major + "." + bank.minor + "." +
                bank.build + "." + bank.rc
        if (bank.c3Revision > 0) v += "-C3." + bank.c3Revision
        return v
    }

    function bankCrcText(bank) {
        if (!bank) return ""
        if (isBankEmpty(bank)) return "No Firmware"
        return bank.crcValid ? "CRC: Valid" : "CRC: Invalid"
    }

    function bankCrcColor(bank) {
        if (!bank || isBankEmpty(bank)) return Material.hintTextColor
        return bank.crcValid ? "#4CAF50" : "#F44336"
    }

    function bankSizeText(bank) {
        if (!bank || isBankEmpty(bank)) return ""
        if (bank.imageSize > 0)
            return "Size: " + (bank.imageSize / 1024).toFixed(0) + " KB"
        return "Size: Unknown"
    }

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
        function onBankEraseComplete() {
            view.eraseInProgress = false
            view.swapStatus = "Inactive bank wiped successfully."
            view.swapStatusColor = "#4CAF50"
            m1device.requestFwInfo()
        }
        function onBankEraseError(message) {
            view.eraseInProgress = false
            view.swapStatus = "Bank erase failed: " + message
            view.swapStatusColor = "#F44336"
        }
        function onConnectionChanged() {
            if (m1device.connected) {
                view.fwInfo = null
                view.swapStatus = ""
                view.eraseInProgress = false
                m1device.requestFwInfo()
            } else if (view.eraseInProgress) {
                view.eraseInProgress = false
                view.swapStatus = "Connection lost during erase."
                view.swapStatusColor = "#F44336"
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
                        text: bankVersionText(fwInfo ? fwInfo.bank1 : null)
                        font.pixelSize: 16
                        color: isBankEmpty(fwInfo ? fwInfo.bank1 : null) ? Material.hintTextColor : Material.foreground
                    }

                    Label {
                        text: bankCrcText(fwInfo ? fwInfo.bank1 : null)
                        color: bankCrcColor(fwInfo ? fwInfo.bank1 : null)
                    }

                    Label {
                        visible: bankSizeText(fwInfo ? fwInfo.bank1 : null).length > 0
                        text: bankSizeText(fwInfo ? fwInfo.bank1 : null)
                        font.pixelSize: 11
                        color: Material.hintTextColor
                    }

                    Label {
                        visible: fwInfo && fwInfo.bank1 && !isBankEmpty(fwInfo.bank1) && fwInfo.bank1.buildDate.length > 0
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
                        if (isBankEmpty(inactive)) return false
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
                        text: bankVersionText(fwInfo ? fwInfo.bank2 : null)
                        font.pixelSize: 16
                        color: isBankEmpty(fwInfo ? fwInfo.bank2 : null) ? Material.hintTextColor : Material.foreground
                    }

                    Label {
                        text: bankCrcText(fwInfo ? fwInfo.bank2 : null)
                        color: bankCrcColor(fwInfo ? fwInfo.bank2 : null)
                    }

                    Label {
                        visible: bankSizeText(fwInfo ? fwInfo.bank2 : null).length > 0
                        text: bankSizeText(fwInfo ? fwInfo.bank2 : null)
                        font.pixelSize: 11
                        color: Material.hintTextColor
                    }

                    Label {
                        visible: fwInfo && fwInfo.bank2 && !isBankEmpty(fwInfo.bank2) && fwInfo.bank2.buildDate.length > 0
                        text: "Built: " + (fwInfo && fwInfo.bank2 ? fwInfo.bank2.buildDate : "")
                        font.pixelSize: 11
                        color: Material.hintTextColor
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }

        // Action buttons
        RowLayout {
            visible: m1device.connected
            Layout.alignment: Qt.AlignHCenter
            spacing: 16

            Button {
                text: "Refresh Bank Info"
                onClicked: m1device.requestFwInfo()
            }

            Button {
                text: view.eraseInProgress ? "Erasing..." : "Wipe Inactive Bank"
                enabled: m1device.connected && !view.eraseInProgress
                Material.foreground: "#F44336"
                onClicked: eraseConfirm.open()
            }
        }

        BusyIndicator {
            visible: view.eraseInProgress
            Layout.alignment: Qt.AlignHCenter
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

    Dialog {
        id: eraseConfirm
        title: "Wipe Inactive Bank"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.No

        Label {
            text: "This will permanently erase all firmware in Bank " +
                  (m1device.activeBank === 1 ? "2" : "1") +
                  " (the inactive bank).\n\n" +
                  "The bank will be empty and cannot be swapped to until " +
                  "new firmware is uploaded.\n\n" +
                  "This operation takes a few seconds."
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            view.eraseInProgress = true
            view.swapStatus = "Erasing inactive bank... this may take a few seconds."
            view.swapStatusColor = Material.foreground
            m1device.eraseInactiveBank()
        }
    }

    Component.onCompleted: {
        if (m1device.connected) m1device.requestFwInfo()
    }
}
