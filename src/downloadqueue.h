#pragma once

#include <QByteArray>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QQueue>
#include <QUrl>
#include <functional>

class DownloadQueue : public QObject {
  Q_OBJECT

 public:
  using StartedCallback = std::function<void()>;
  using ProgressCallback = std::function<void(qint64 bytesReceived, qint64 bytesTotal)>;
  using FinishedCallback = std::function<void(const QByteArray &data, bool success, const QString &errorString)>;

  explicit DownloadQueue(int maxConcurrent = 1, QObject *parent = nullptr);
  ~DownloadQueue();

  void enqueue(const QUrl &url, StartedCallback startedCallback, ProgressCallback progressCallback,
               FinishedCallback finishedCallback);
  bool isEmpty() const;
  int pendingCount() const;
  int activeCount() const;
  bool isDownloading() const;

 private:
  struct DownloadItem {
    QUrl url;
    StartedCallback startedCallback;
    ProgressCallback progressCallback;
    FinishedCallback finishedCallback;
  };

  void startNext();

  QNetworkAccessManager *m_networkManager;
  QQueue<DownloadItem> m_queue;
  QMap<QNetworkReply *, DownloadItem> m_activeDownloads;
  int m_maxConcurrent;
};
