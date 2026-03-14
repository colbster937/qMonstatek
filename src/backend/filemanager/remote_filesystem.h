/*
 * remote_filesystem.h — Remote file operations on the M1 SD card
 *
 * Provides an interface for browsing, uploading, downloading, and
 * managing files on the M1 device's SD card over the serial RPC link.
 */

#ifndef REMOTE_FILESYSTEM_H
#define REMOTE_FILESYSTEM_H

#include <QObject>
#include <QString>
#include <QJsonArray>

class M1Device;

class RemoteFilesystem : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    explicit RemoteFilesystem(QObject *parent = nullptr);

    /**
     * Set the device to operate on.
     */
    void setDevice(M1Device *device);

    /**
     * List directory contents on the device.
     */
    Q_INVOKABLE void listDirectory(const QString &path = "/");

    /**
     * Navigate into a subdirectory.
     */
    Q_INVOKABLE void navigateTo(const QString &path);

    /**
     * Navigate up one directory level.
     */
    Q_INVOKABLE void navigateUp();

    /**
     * Download a file from the device to a local path.
     */
    Q_INVOKABLE void downloadFile(const QString &remotePath, const QString &localPath);

    /**
     * Upload a local file to the device.
     */
    Q_INVOKABLE void uploadFile(const QString &localPath, const QString &remotePath);

    /**
     * Delete a file or directory on the device.
     */
    Q_INVOKABLE void deleteItem(const QString &remotePath);

    /**
     * Create a directory on the device.
     */
    Q_INVOKABLE void createDirectory(const QString &remotePath);

    QString currentPath() const { return m_currentPath; }
    bool isBusy() const { return m_busy; }

signals:
    void currentPathChanged(const QString &path);
    void busyChanged(bool busy);
    void directoryListed(const QString &path, const QJsonArray &entries);
    void transferProgress(int percent);
    void transferComplete(const QString &path);
    void operationError(const QString &message);

private:
    M1Device *m_device = nullptr;
    QString   m_currentPath = "/";
    bool      m_busy = false;
};

#endif // REMOTE_FILESYSTEM_H
