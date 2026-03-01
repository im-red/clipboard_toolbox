#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QUrl>

struct DownloadProgressItem {
  int id = 0;
  QUrl url;
  qint64 bytesReceived = 0;
  qint64 bytesTotal = 0;
  int progress = 0;
  QString status;
  bool isFinished = false;
  bool isError = false;
  bool isQueued = false;
  bool isConnecting = false;
};

class DownloadProgressModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Roles {
    IdRole = Qt::UserRole + 1,
    UrlRole,
    ProgressRole,
    StatusRole,
    BytesReceivedRole,
    BytesTotalRole,
    IsFinishedRole,
    IsErrorRole,
    IsQueuedRole,
    IsConnectingRole
  };

  explicit DownloadProgressModel(QObject *parent = nullptr);
  ~DownloadProgressModel();

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  int addQueuedDownload(const QUrl &url);
  void setConnecting(int id);
  void updateProgress(int id, qint64 bytesReceived, qint64 bytesTotal);
  void setFinished(int id, bool success);
  void clearFinished();

 private:
  QList<DownloadProgressItem> m_downloads;
  int m_nextId = 1;
  int findRowById(int id);
};
