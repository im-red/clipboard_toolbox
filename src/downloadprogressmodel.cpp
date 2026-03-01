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
    case ReplyRole:
      return QVariant::fromValue(item.reply);
  }

  return QVariant();
}

QHash<int, QByteArray> DownloadProgressModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[Qt::DisplayRole] = "display";
  roles[UrlRole] = "url";
  roles[ProgressRole] = "progress";
  roles[StatusRole] = "status";
  roles[BytesReceivedRole] = "bytesReceived";
  roles[BytesTotalRole] = "bytesTotal";
  roles[IsFinishedRole] = "isFinished";
  roles[IsErrorRole] = "isError";
  return roles;
}

void DownloadProgressModel::addDownload(QNetworkReply *reply, const QUrl &url) {
  beginInsertRows(QModelIndex(), 0, 0);  // Insert at top
  DownloadProgressItem item;
  item.url = url;
  item.reply = reply;
  item.status = "Connecting...";
  m_downloads.prepend(item);
  endInsertRows();

  connect(reply, &QNetworkReply::downloadProgress, this, &DownloadProgressModel::onDownloadProgress);
  connect(reply, &QNetworkReply::finished, this, &DownloadProgressModel::onFinished);
  connect(reply, &QObject::destroyed, this, &DownloadProgressModel::onReplyDestroyed);
}

void DownloadProgressModel::removeDownload(int index) {
  if (index < 0 || index >= m_downloads.count()) return;

  beginRemoveRows(QModelIndex(), index, index);
  m_downloads.removeAt(index);
  endRemoveRows();
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

void DownloadProgressModel::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  int row = findRowByReply(reply);
  if (row < 0) return;

  DownloadProgressItem &item = m_downloads[row];
  item.bytesReceived = bytesReceived;
  item.bytesTotal = bytesTotal;

  if (bytesTotal > 0) {
    item.progress = (int)((bytesReceived * 100) / bytesTotal);
    item.status = QString("%1/%2").arg(utils::formatSize(bytesReceived)).arg(utils::formatSize(bytesTotal));
  } else {
    item.progress = 0;
    item.status = QString("%1").arg(utils::formatSize(bytesReceived));
  }

  emit dataChanged(index(row), index(row), {ProgressRole, StatusRole, BytesReceivedRole, BytesTotalRole});
}

void DownloadProgressModel::onFinished() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  int row = findRowByReply(reply);
  if (row < 0) return;

  DownloadProgressItem &item = m_downloads[row];
  item.isFinished = true;

  if (reply->error() != QNetworkReply::NoError) {
    item.isError = true;
    item.status = "Error";
  } else {
    item.progress = 100;
    item.status = QString("%1 (Done)").arg(utils::formatSize(item.bytesReceived));
  }

  emit dataChanged(index(row), index(row), {ProgressRole, StatusRole, IsFinishedRole, IsErrorRole});
}

void DownloadProgressModel::onReplyDestroyed(QObject *obj) {
  // Reply might be deleted externally or auto-deleted
  // We keep the record in model until explicitly removed, but clear pointer
  // Actually, if we want to remove from list when reply is destroyed, we can do it here.
  // But usually we want to keep "Finished" state visible.
  // So just nullify the pointer to avoid crash if accessed.
  /*
  for (int i = 0; i < m_downloads.count(); ++i) {
  if (m_downloads[i].reply == obj) {
      m_downloads[i].reply = nullptr;
      break;
  }
  }
  */
}

int DownloadProgressModel::findRowByReply(QNetworkReply *reply) {
  for (int i = 0; i < m_downloads.count(); ++i) {
    if (m_downloads[i].reply == reply) return i;
  }
  return -1;
}
