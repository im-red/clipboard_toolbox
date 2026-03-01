#include "downloadprogressmodel.h"

#include "utils.h"

DownloadProgressModel::DownloadProgressModel(QObject *parent) : QAbstractListModel(parent) {}

DownloadProgressModel::~DownloadProgressModel() {}

int DownloadProgressModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) return 0;
  return m_downloads.count();
}

QVariant DownloadProgressModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_downloads.count()) return QVariant();

  const DownloadProgressItem &item = m_downloads[index.row()];

  switch (role) {
    case Qt::DisplayRole:
      return item.url.fileName().isEmpty() ? item.url.toString() : item.url.fileName();
    case IdRole:
      return item.id;
    case UrlRole:
      return item.url;
    case ProgressRole:
      return item.progress;
    case StatusRole:
      return item.status;
    case BytesReceivedRole:
      return item.bytesReceived;
    case BytesTotalRole:
      return item.bytesTotal;
    case IsFinishedRole:
      return item.isFinished;
    case IsErrorRole:
      return item.isError;
    case IsQueuedRole:
      return item.isQueued;
    case IsConnectingRole:
      return item.isConnecting;
  }

  return QVariant();
}

QHash<int, QByteArray> DownloadProgressModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[Qt::DisplayRole] = "display";
  roles[IdRole] = "id";
  roles[UrlRole] = "url";
  roles[ProgressRole] = "progress";
  roles[StatusRole] = "status";
  roles[BytesReceivedRole] = "bytesReceived";
  roles[BytesTotalRole] = "bytesTotal";
  roles[IsFinishedRole] = "isFinished";
  roles[IsErrorRole] = "isError";
  roles[IsQueuedRole] = "isQueued";
  roles[IsConnectingRole] = "isConnecting";
  return roles;
}

int DownloadProgressModel::addQueuedDownload(const QUrl &url) {
  beginInsertRows(QModelIndex(), 0, 0);
  DownloadProgressItem item;
  item.id = m_nextId++;
  item.url = url;
  item.isQueued = true;
  item.status = "Queued";
  m_downloads.prepend(item);
  endInsertRows();
  return item.id;
}

void DownloadProgressModel::setConnecting(int id) {
  int row = findRowById(id);
  if (row < 0) return;

  DownloadProgressItem &item = m_downloads[row];
  item.isQueued = false;
  item.isConnecting = true;
  item.status = "Connecting...";

  emit dataChanged(index(row), index(row), {StatusRole, IsQueuedRole, IsConnectingRole});
}

void DownloadProgressModel::updateProgress(int id, qint64 bytesReceived, qint64 bytesTotal) {
  int row = findRowById(id);
  if (row < 0) return;

  DownloadProgressItem &item = m_downloads[row];
  item.isQueued = false;
  item.isConnecting = false;
  item.bytesReceived = bytesReceived;
  item.bytesTotal = bytesTotal;

  if (bytesTotal > 0) {
    item.progress = (int)((bytesReceived * 100) / bytesTotal);
    item.status = QString("%1/%2").arg(utils::formatSize(bytesReceived)).arg(utils::formatSize(bytesTotal));
  } else {
    item.progress = 0;
    item.status = QString("%1").arg(utils::formatSize(bytesReceived));
  }

  emit dataChanged(index(row), index(row),
                   {ProgressRole, StatusRole, BytesReceivedRole, BytesTotalRole, IsQueuedRole, IsConnectingRole});
}

void DownloadProgressModel::setFinished(int id, bool success) {
  int row = findRowById(id);
  if (row < 0) return;

  DownloadProgressItem &item = m_downloads[row];
  item.isFinished = true;
  item.isQueued = false;
  item.isConnecting = false;

  if (!success) {
    item.isError = true;
    item.status = "Error";
  } else {
    item.progress = 100;
    item.status = QString("%1 (Done)").arg(utils::formatSize(item.bytesReceived));
  }

  emit dataChanged(index(row), index(row),
                   {ProgressRole, StatusRole, IsFinishedRole, IsErrorRole, IsQueuedRole, IsConnectingRole});
}

void DownloadProgressModel::clearFinished() {
  for (int i = m_downloads.count() - 1; i >= 0; --i) {
    if (m_downloads[i].isFinished) {
      beginRemoveRows(QModelIndex(), i, i);
      m_downloads.removeAt(i);
      endRemoveRows();
    }
  }
}

int DownloadProgressModel::findRowById(int id) {
  for (int i = 0; i < m_downloads.count(); ++i) {
    if (m_downloads[i].id == id) return i;
  }
  return -1;
}
