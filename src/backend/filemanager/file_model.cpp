/*
 * file_model.cpp — List model for remote filesystem entries
 */

#include "file_model.h"

FileModel::FileModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_entries.size());
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const FileEntry &entry = m_entries.at(index.row());

    switch (role) {
    case NameRole:        return entry.name;
    case PathRole:        return entry.path;
    case SizeRole:        return static_cast<qulonglong>(entry.size);
    case IsDirectoryRole: return entry.isDirectory;
    case ModifiedRole:    return entry.modified;
    case IconRole:        return entry.isDirectory ? "folder" : "file";
    default:              return {};
    }
}

QHash<int, QByteArray> FileModel::roleNames() const
{
    return {
        { NameRole,        "name"        },
        { PathRole,        "path"        },
        { SizeRole,        "size"        },
        { IsDirectoryRole, "isDirectory" },
        { ModifiedRole,    "modified"    },
        { IconRole,        "icon"        }
    };
}

void FileModel::setEntries(const QJsonArray &entries)
{
    beginResetModel();
    m_entries.clear();

    for (const QJsonValue &v : entries) {
        QJsonObject map = v.toObject();
        FileEntry e;
        e.name        = map.value("name").toString();
        e.path        = map.value("path").toString();
        e.size        = static_cast<quint64>(map.value("size").toDouble());
        e.isDirectory = map.value("isDirectory").toBool();
        e.modified    = map.value("modified").toString();
        m_entries.append(e);
    }

    endResetModel();
    emit countChanged();
}

void FileModel::clear()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

void FileModel::setCurrentPath(const QString &path)
{
    if (m_currentPath != path) {
        m_currentPath = path;
        emit currentPathChanged(m_currentPath);
    }
}
