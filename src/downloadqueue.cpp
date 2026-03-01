#include "downloadqueue.h"

DownloadQueue::DownloadQueue(int maxConcurrent, QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)), m_maxConcurrent(maxConcurrent) {}

DownloadQueue::~DownloadQueue() {}

void DownloadQueue::enqueue(const QUrl &url, StartedCallback startedCallback, ProgressCallback progressCallback,
                            FinishedCallback finishedCallback) {
  m_queue.enqueue({url, startedCallback, progressCallback, finishedCallback});
  startNext();
}

bool DownloadQueue::isEmpty() const { return m_queue.isEmpty() && m_activeDownloads.isEmpty(); }

int DownloadQueue::pendingCount() const { return m_queue.count(); }

int DownloadQueue::activeCount() const { return m_activeDownloads.count(); }

bool DownloadQueue::isDownloading() const { return !m_activeDownloads.isEmpty(); }

void DownloadQueue::startNext() {
  while (m_queue.count() > 0 && m_activeDownloads.count() < m_maxConcurrent) {
    DownloadItem item = m_queue.dequeue();

    QNetworkRequest request(item.url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (compatible; ClipboardToolbox/1.0)");
    request.setTransferTimeout(60000);

    QNetworkReply *reply = m_networkManager->get(request);

    if (item.startedCallback) {
      item.startedCallback();
    }

    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 bytesReceived, qint64 bytesTotal) {
      if (!m_activeDownloads.contains(reply)) return;
      DownloadItem &downloadItem = m_activeDownloads[reply];
      if (downloadItem.progressCallback) {
        downloadItem.progressCallback(bytesReceived, bytesTotal);
      }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
      if (!m_activeDownloads.contains(reply)) return;

      DownloadItem item = m_activeDownloads.take(reply);

      QByteArray data;
      bool success = false;
      QString errorString;

      if (reply->error() == QNetworkReply::NoError) {
        data = reply->readAll();
        success = true;
      } else {
        errorString = reply->errorString();
      }

      reply->deleteLater();

      if (item.finishedCallback) {
        item.finishedCallback(data, success, errorString);
      }

      startNext();
    });

    m_activeDownloads.insert(reply, item);
  }
}
