/*
 * firmware_updater.h — STM32 firmware update manager
 *
 * Handles firmware binary validation and OTA update to the M1 device
 * over the serial RPC link.
 */

#ifndef FIRMWARE_UPDATER_H
#define FIRMWARE_UPDATER_H

#include <QObject>
#include <QString>

class M1Device;

class FirmwareUpdater : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool updating READ isUpdating NOTIFY updatingChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

public:
    explicit FirmwareUpdater(QObject *parent = nullptr);

    /**
     * Set the device to update.
     */
    void setDevice(M1Device *device);

    /**
     * Start firmware update from a local .bin file.
     */
    Q_INVOKABLE void startUpdate(const QString &binFilePath);

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

#endif // FIRMWARE_UPDATER_H
