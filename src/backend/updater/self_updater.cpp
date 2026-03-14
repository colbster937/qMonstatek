/*
 * self_updater.cpp — qMonstatek desktop application self-updater
 */

#include "self_updater.h"

#include <QDebug>

SelfUpdater::SelfUpdater(QObject *parent)
    : QObject(parent)
{
}

void SelfUpdater::checkForUpdates()
{
    if (m_checking) return;

    m_checking = true;
    emit checkingChanged(m_checking);

    // TODO: query GitHub releases API for qMonstatek
    // For now, emit no update available
    m_checking = false;
    emit checkingChanged(m_checking);
}

void SelfUpdater::downloadAndInstall()
{
    if (!m_updateAvailable) {
        emit updateError("No update available");
        return;
    }

    // TODO: download installer and launch
    emit updateError("Self-update not yet implemented");
}
