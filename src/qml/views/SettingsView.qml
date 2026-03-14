import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Item {
    id: view

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: "Settings"
            font.pixelSize: 24
            font.bold: true
        }

        // GitHub Repository
        GroupBox {
            title: "Firmware Update Source"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                Label {
                    text: "GitHub repository for firmware updates (format: owner/repo)"
                    font.pixelSize: 12
                    color: Material.hintTextColor
                }

                RowLayout {
                    TextField {
                        id: repoField
                        text: githubChecker.repoUrl
                        placeholderText: "owner/repo"
                        Layout.fillWidth: true
                    }
                    Button {
                        text: "Save"
                        onClicked: githubChecker.repoUrl = repoField.text
                    }
                    Button {
                        text: "Reset"
                        onClicked: {
                            repoField.text = "bedge117/M1"
                            githubChecker.repoUrl = "bedge117/M1"
                        }
                    }
                }
            }
        }

        // Screen Mirror Settings
        GroupBox {
            title: "Screen Mirror"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                RowLayout {
                    Label { text: "Default FPS:" }
                    SpinBox {
                        id: defaultFps
                        from: 1; to: 15; value: 10
                    }
                }

                Label {
                    text: "Higher FPS uses more bandwidth. 10 FPS recommended."
                    font.pixelSize: 11
                    color: Material.hintTextColor
                }
            }
        }

        // Connection Settings
        GroupBox {
            title: "Connection"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                CheckBox {
                    id: autoConnectCheck
                    text: "Auto-connect when M1 device is detected"
                    checked: true
                }

                Label {
                    text: "When enabled, qMonstatek will automatically connect to " +
                          "the first M1 device detected on USB."
                    font.pixelSize: 11
                    color: Material.hintTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // Theme
        GroupBox {
            title: "Appearance"
            Layout.fillWidth: true

            RowLayout {
                Label { text: "Theme:" }
                RadioButton {
                    text: "Dark"
                    checked: true
                }
                RadioButton {
                    text: "Light"
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
