/*
 * file_model.h — List model for remote filesystem entries
 *
 * Provides a QAbstractListModel for displaying directory contents
 * from the M1 device's SD card in QML views.
 */

#ifndef FILE_MODEL_H
#define FILE_MODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QJsonArray>
#include <QJsonObject>

class FileModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ currentPath WRITE setCurrentPath NOTIFY currentPathChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum FileRoles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SizeRole,
        IsDirectoryRole,
        ModifiedRole,
        IconRole
    };
    Q_ENUM(FileRoles)

    explicit FileModel(QObject *parent = nullptr);

    /* QAbstractListModel overrides */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * Populate the model with file entries from a directory listing response.
     */
    Q_INVOKABLE void setEntries(const QJsonArray &entries);

    /**
     * Clear all entries.
     */
    Q_INVOKABLE void clear();

    QString currentPath() const { return m_currentPath; }
    void setCurrentPath(const QString &path);

signals:
    void currentPathChanged(const QString &path);
    void countChanged();

private:
    struct FileEntry {
        QString  name;
        QString  path;
        quint64  size = 0;
        bool     isDirectory = false;
        QString  modified;
    };

    QVector<FileEntry> m_entries;
    QString            m_currentPath = "/";
};

#endif // FILE_MODEL_H
