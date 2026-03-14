/*
 * esp32_updater.cpp — ESP32 coprocessor firmware updater
 */

#include "esp32_updater.h"

#include <QDebug>

Esp32Updater::Esp32Updater(QObject *parent)
    : QObject(parent)
{
}

void Esp32Updater::setDevice(M1Device *device)
{
    m_device = device;
}

void Esp32Updater::startUpdate(const QString &binFilePath, uint32_t flashAddr)
{
    Q_UNUSED(binFilePath);
    Q_UNUSED(flashAddr);

    if (m_updating) {
        qWarning() << "Esp32Updater: update already in progress";
        return;
    }

    if (!m_device) {
        emit updateError("No device connected");
        return;
    }

    // TODO: implement ESP32 flash sequence
    m_updating = true;
    m_progress = 0;
    emit updatingChanged(m_updating);
    emit progressChanged(m_progress);
}

void Esp32Updater::cancel()
{
    if (!m_updating) return;

    m_updating = false;
    m_progress = 0;
    emit updatingChanged(m_updating);
    emit progressChanged(m_progress);
}
