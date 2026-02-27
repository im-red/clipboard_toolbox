#pragma once

#include <QAbstractListModel>
#include <QNetworkReply>
#include <QUrl>
#include <QList>
#include <QObject>

struct DownloadProgressItem {
  QUrl url;
  qint64 bytesReceived = 0;
  qint64 bytesTotal = 0;
  int progress = 0;
  QString status;
  bool isFinished = false;
  bool isError = false;
  QNetworkReply *reply = nullptr;
};

class DownloadProgressModel : public QAbstractListModel {
  Q_OBJECT

 public:
  enum Roles {
    UrlRole = Qt::UserRole + 1,
    ProgressRole,
    StatusRole,
    BytesReceivedRole,
    BytesTotalRole,
    IsFinishedRole,
    IsErrorRole,
    ReplyRole
  };

  explicit DownloadProgressModel(QObject *parent = nullptr);
  ~DownloadProgressModel();

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  void addDownload(QNetworkReply *reply, const QUrl &url);
  void removeDownload(int index);
  void clearFinished();

 private slots:
  void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
  void onFinished();
  void onReplyDestroyed(QObject *obj);

 private:
  QList<DownloadProgressItem> m_downloads;
  int findRowByReply(QNetworkReply *reply);
};
