/*
 * serial_transport.h — QSerialPort wrapper for M1 communication
 */

#ifndef SERIAL_TRANSPORT_H
#define SERIAL_TRANSPORT_H

#include <QObject>
#include <QSerialPort>
#include <QString>

class SerialTransport : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)

public:
    explicit SerialTransport(QObject *parent = nullptr);
    ~SerialTransport() override;

    /**
     * Open a serial connection to the specified port.
     * @param portName  COM port name (e.g., "COM3")
     * @return true on success
     */
    bool open(const QString &portName);

    /**
     * Close the serial connection.
     */
    void close();

    /**
     * Send raw bytes to the device.
     */
    qint64 send(const QByteArray &data);

    bool isConnected() const;
    QString portName() const;

signals:
    void dataReceived(const QByteArray &data);
    void connectionChanged(bool connected);
    void errorOccurred(const QString &message);

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);

private:
    QSerialPort m_port;
};

#endif // SERIAL_TRANSPORT_H
