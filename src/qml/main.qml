import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "components"
import "views"

ApplicationWindow {
    id: root
    width: 960
    height: 640
    minimumWidth: 800
    minimumHeight: 560
    visible: true
    title: "qMonstatek" + (m1device.connected ? " — " + m1device.portName : "")

    Material.theme: Material.Dark
    Material.accent: Material.Green
    Material.primary: Material.BlueGrey

    // ── Status Bar ──
    header: StatusBar {
        id: statusBar
    }

    // ── Main Layout: Sidebar + Content ──
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Sidebar ──
        Sidebar {
            id: sidebar
            Layout.fillHeight: true
            onNavigated: function(viewName) {
                contentStack.currentIndex = viewIndex(viewName)
                refreshView(viewName)
            }
        }

        // ── Separator ──
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: Material.dividerColor
        }

        // ── Content Area ──
        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true

            DeviceInfoView     { id: deviceInfoView }
            ScreenMirrorView   { id: screenMirrorView }
            FileManagerView    { id: fileManagerView }
            FirmwareUpdateView { id: fwUpdateView }
            Esp32UpdateView    { id: espUpdateView }
            DualBootView       { id: dualBootView }
            DebugTerminalView  { id: debugTerminalView }
            SettingsView       { id: settingsView }
            PowerView          { id: powerView }
            AboutView          { id: aboutView }
            DfuFlashView       { id: dfuFlashView }
            SwdRecoveryView    { id: swdRecoveryView }
        }
    }

    // ── Device selector dialog ──
    DeviceSelector {
        id: deviceSelector
    }

    // ── View index mapping ──
    function viewIndex(name) {
        switch (name) {
            case "deviceInfo":      return 0
            case "screenMirror":    return 1
            case "fileManager":     return 2
            case "firmwareUpdate":  return 3
            case "esp32Update":     return 4
            case "dualBoot":        return 5
            case "debugTerminal":   return 6
            case "settings":        return 7
            case "power":           return 8
            case "about":           return 9
            case "dfuFlash":        return 10
            case "swdRecovery":     return 11
            default:                return 0
        }
    }

    // ── Auto-refresh the active view when device connects ──
    Connections {
        target: m1device
        function onConnectionChanged(connected) {
            if (connected) {
                m1device.requestDeviceInfo()
                refreshCurrentView()
                // Safety net: M1's USB CDC re-enumerates before RPC is ready
                // after reboot/bank-swap.  Retry after 2s to catch the slow case.
                reconnectRefreshTimer.restart()
            } else {
                reconnectRefreshTimer.stop()
            }
        }
    }

    Timer {
        id: reconnectRefreshTimer
        interval: 2000
        onTriggered: {
            if (m1device.connected) {
                m1device.requestDeviceInfo()
                refreshCurrentView()
            }
        }
    }

    function refreshCurrentView() {
        var names = ["deviceInfo", "screenMirror", "fileManager",
                     "firmwareUpdate", "esp32Update", "dualBoot",
                     "debugTerminal", "settings", "power", "about",
                     "dfuFlash", "swdRecovery"]
        refreshView(names[contentStack.currentIndex] || "")
    }

    function refreshView(viewName) {
        if (!m1device.connected) return
        switch (viewName) {
            case "deviceInfo":
                m1device.requestDeviceInfo()
                break
            case "fileManager":
                fileManagerView.refresh()
                break
            case "dualBoot":
                m1device.requestFwInfo()
                break
            case "esp32Update":
                m1device.requestDeviceInfo()
                m1device.requestEspInfo()
                break
            // screenMirror, firmwareUpdate, settings, about: no auto-refresh needed
        }
    }

    // ── Keyboard shortcuts for remote control ──
    Shortcut {
        sequence: "Up"
        enabled: contentStack.currentIndex === 1 && m1device.connected
        onActivated: m1device.buttonClick(1)  // BUTTON_UP
    }
    Shortcut {
        sequence: "Down"
        enabled: contentStack.currentIndex === 1 && m1device.connected
        onActivated: m1device.buttonClick(4)  // BUTTON_DOWN
    }
    Shortcut {
        sequence: "Left"
        enabled: contentStack.currentIndex === 1 && m1device.connected
        onActivated: m1device.buttonClick(2)  // BUTTON_LEFT
    }
    Shortcut {
        sequence: "Right"
        enabled: contentStack.currentIndex === 1 && m1device.connected
        onActivated: m1device.buttonClick(3)  // BUTTON_RIGHT
    }
    Shortcut {
        sequence: "Return"
        enabled: contentStack.currentIndex === 1 && m1device.connected
        onActivated: m1device.buttonClick(0)  // BUTTON_OK
    }
    Shortcut {
        sequence: "Escape"
        enabled: contentStack.currentIndex === 1 && m1device.connected
        onActivated: m1device.buttonClick(5)  // BUTTON_BACK
    }
}
