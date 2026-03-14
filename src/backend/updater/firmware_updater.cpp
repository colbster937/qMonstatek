/*
 * firmware_updater.cpp — STM32 firmware update manager
 */

#include "firmware_updater.h"

#include <QDebug>

FirmwareUpdater::FirmwareUpdater(QObject *parent)
    : QObject(parent)
{
}

void FirmwareUpdater::setDevice(M1Device *device)
{
    m_device = device;
}

void FirmwareUpdater::startUpdate(const QString &binFilePath)
{
    Q_UNUSED(binFilePath);

    if (m_updating) {
        qWarning() << "FirmwareUpdater: update already in progress";
        return;
    }

    if (!m_device) {
        emit updateError("No device connected");
        return;
    }

    // TODO: implement firmware update sequence
    m_updating = true;
    m_progress = 0;
    emit updatingChanged(m_updating);
    emit progressChanged(m_progress);
}

void FirmwareUpdater::cancel()
{
    if (!m_updating) return;

    m_updating = false;
    m_progress = 0;
    emit updatingChanged(m_updating);
    emit progressChanged(m_progress);
}
