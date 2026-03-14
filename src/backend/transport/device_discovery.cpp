/*
 * device_discovery.cpp — M1 device auto-detection
 */

#include "device_discovery.h"

#include <QSerialPortInfo>
#include <QDebug>

DeviceDiscovery::DeviceDiscovery(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &DeviceDiscovery::scan);
}

void DeviceDiscovery::startScanning()
{
    scan();  // immediate first scan
    m_timer.start(2000);  // then every 2 seconds
}

void DeviceDiscovery::stopScanning()
{
    m_timer.stop();
}

void DeviceDiscovery::scan()
{
    QStringList currentPorts;

    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &info : ports) {
        if (info.hasVendorIdentifier() && info.hasProductIdentifier()) {
            if (info.vendorIdentifier() == M1_VID &&
                (info.productIdentifier() == M1_PID_CDC_MSC ||
                 info.productIdentifier() == M1_PID_CDC)) {
                currentPorts.append(info.portName());
            }
        }
    }

    // Detect new devices
    for (const auto &port : currentPorts) {
        if (!m_ports.contains(port)) {
            qInfo() << "M1 device found on" << port;
            emit deviceFound(port);
        }
    }

    // Detect lost devices
    for (const auto &port : m_ports) {
        if (!currentPorts.contains(port)) {
            qInfo() << "M1 device lost on" << port;
            emit deviceLost(port);
        }
    }

    if (currentPorts != m_ports) {
        m_ports = currentPorts;
        emit portsChanged();
    }
}

void DeviceDiscovery::forgetPort(const QString &portName)
{
    m_ports.removeAll(portName);
}

QStringList DeviceDiscovery::allSerialPorts()
{
    QStringList result;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &info : ports) {
        QString desc = info.portName();
        if (!info.description().isEmpty()) {
            desc += " - " + info.description();
        }
        result.append(desc);
    }
    return result;
}
