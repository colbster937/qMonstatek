/*
 * device_discovery.h — Auto-detect M1 devices on USB serial ports
 *
 * Scans for USB serial ports matching the M1's VID/PID and emits
 * signals when devices appear or disappear.
 */

#ifndef DEVICE_DISCOVERY_H
#define DEVICE_DISCOVERY_H

#include <QObject>
#include <QTimer>
#include <QStringList>

class DeviceDiscovery : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY portsChanged)

public:
    explicit DeviceDiscovery(QObject *parent = nullptr);

    /* M1 USB identifiers */
    static constexpr uint16_t M1_VID = 0x0483;  // STMicroelectronics
    static constexpr uint16_t M1_PID_CDC_MSC = 0x5750;  // CDC+MSC composite
    static constexpr uint16_t M1_PID_CDC     = 0x5740;  // CDC only

    /**
     * Start periodic scanning (every 2 seconds).
     */
    void startScanning();

    /**
     * Stop periodic scanning.
     */
    void stopScanning();

    /**
     * Get list of currently detected M1 port names.
     */
    QStringList availablePorts() const { return m_ports; }

    /**
     * Remove a port from the known-ports list so the next scan
     * re-detects it as a new device (used after unexpected disconnect).
     */
    void forgetPort(const QString &portName);

    /**
     * Get all serial ports (including non-M1) for manual selection.
     */
    static QStringList allSerialPorts();

signals:
    void deviceFound(const QString &portName);
    void deviceLost(const QString &portName);
    void portsChanged();

private slots:
    void scan();

private:
    QTimer      m_timer;
    QStringList m_ports;  // currently detected M1 ports
};

#endif // DEVICE_DISCOVERY_H
