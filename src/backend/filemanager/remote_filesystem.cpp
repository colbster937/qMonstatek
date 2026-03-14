/*
 * remote_filesystem.cpp — Remote file operations on the M1 SD card
 */

#include "remote_filesystem.h"

#include <QDebug>

RemoteFilesystem::RemoteFilesystem(QObject *parent)
    : QObject(parent)
{
}

void RemoteFilesystem::setDevice(M1Device *device)
{
    m_device = device;
}

void RemoteFilesystem::listDirectory(const QString &path)
{
    if (m_busy) {
        qWarning() << "RemoteFilesystem: operation already in progress";
        return;
    }

    if (!m_device) {
        emit operationError("No device connected");
        return;
    }

    // TODO: send directory listing RPC command
    Q_UNUSED(path);
}

void RemoteFilesystem::navigateTo(const QString &path)
{
    m_currentPath = path;
    emit currentPathChanged(m_currentPath);
    listDirectory(m_currentPath);
}

void RemoteFilesystem::navigateUp()
{
    if (m_currentPath == "/") return;

    int lastSlash = m_currentPath.lastIndexOf('/');
    if (lastSlash <= 0) {
        m_currentPath = "/";
    } else {
        m_currentPath = m_currentPath.left(lastSlash);
    }
    emit currentPathChanged(m_currentPath);
    listDirectory(m_currentPath);
}

void RemoteFilesystem::downloadFile(const QString &remotePath, const QString &localPath)
{
    Q_UNUSED(remotePath);
    Q_UNUSED(localPath);

    if (!m_device) {
        emit operationError("No device connected");
        return;
    }

    // TODO: send file download RPC command
}

void RemoteFilesystem::uploadFile(const QString &localPath, const QString &remotePath)
{
    Q_UNUSED(localPath);
    Q_UNUSED(remotePath);

    if (!m_device) {
        emit operationError("No device connected");
        return;
    }

    // TODO: send file upload RPC command
}

void RemoteFilesystem::deleteItem(const QString &remotePath)
{
    Q_UNUSED(remotePath);

    if (!m_device) {
        emit operationError("No device connected");
        return;
    }

    // TODO: send delete RPC command
}

void RemoteFilesystem::createDirectory(const QString &remotePath)
{
    Q_UNUSED(remotePath);

    if (!m_device) {
        emit operationError("No device connected");
        return;
    }

    // TODO: send mkdir RPC command
}
