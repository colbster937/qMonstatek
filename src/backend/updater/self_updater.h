/*
 * self_updater.h — qMonstatek desktop application self-updater
 *
 * Checks GitHub releases for new versions of the qMonstatek desktop
 * application and manages download/install of updates.
 */

#ifndef SELF_UPDATER_H
#define SELF_UPDATER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QVersionNumber>

class SelfUpdater : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool checking READ isChecking NOTIFY checkingChanged)
    Q_PROPERTY(bool updateAvailable READ isUpdateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateAvailableChanged)

public:
    explicit SelfUpdater(QObject *parent = nullptr);

    /**
     * Check GitHub for a newer version of qMonstatek.
     */
    Q_INVOKABLE void checkForUpdates();

    /**
     * Download and install the latest release.
     */
    Q_INVOKABLE void downloadAndInstall();

    bool isChecking() const { return m_checking; }
    bool isUpdateAvailable() const { return m_updateAvailable; }
    QString latestVersion() const { return m_latestVersion; }
    QString currentVersion() const { return m_currentVersion; }

signals:
    void checkingChanged(bool checking);
    void updateAvailableChanged(bool available);
    void downloadProgress(int percent);
    void downloadComplete(const QString &installerPath);
    void updateError(const QString &message);

private:
    QNetworkAccessManager m_nam;
    QString m_currentVersion = "0.1.0";
    QString m_latestVersion;
    bool    m_checking = false;
    bool    m_updateAvailable = false;
};

#endif // SELF_UPDATER_H
