/*
 * esp32_updater.h — ESP32 coprocessor firmware updater
 *
 * Manages flashing new AT firmware to the ESP32-C6 WiFi coprocessor
 * via the STM32 RPC bridge.
 */

#ifndef ESP32_UPDATER_H
#define ESP32_UPDATER_H

#include <QObject>
#include <QString>

class M1Device;

class Esp32Updater : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool updating READ isUpdating NOTIFY updatingChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

public:
    explicit Esp32Updater(QObject *parent = nullptr);

    /**
     * Set the device whose ESP32 will be updated.
     */
    void setDevice(M1Device *device);

    /**
     * Start ESP32 firmware update from a local .bin file.
     * @param binFilePath  Path to the ESP32 firmware binary
     * @param flashAddr    Target flash address (default 0x0)
     */
    Q_INVOKABLE void startUpdate(const QString &binFilePath, uint32_t flashAddr = 0x0);

    /**
     * Cancel an in-progress update.
     */
    Q_INVOKABLE void cancel();

    bool isUpdating() const { return m_updating; }
    int progress() const { return m_progress; }

signals:
    void updatingChanged(bool updating);
    void progressChanged(int percent);
    void updateComplete();
    void updateError(const QString &message);

private:
    M1Device *m_device = nullptr;
    bool      m_updating = false;
    int       m_progress = 0;
};

#endif // ESP32_UPDATER_H
