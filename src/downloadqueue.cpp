#include "downloadqueue.h"

DownloadQueue::DownloadQueue(QObject *parent) : QObject(parent), m_networkManager(new QNetworkAccessManager(this)) {}

DownloadQueue::~DownloadQueue() {}

void DownloadQueue::enqueue(const QUrl &url, StartedCallback startedCallback, ProgressCallback progressCallback,
                            FinishedCallback finishedCallback) {
  m_queue.enqueue({url, startedCallback, progressCallback, finishedCallback});
  if (!m_currentReply) {
    startNext();
  }
}

bool DownloadQueue::isEmpty() const { return m_queue.isEmpty(); }

int DownloadQueue::pendingCount() const { return m_queue.count(); }

bool DownloadQueue::isDownloading() const { return m_currentReply != nullptr; }

void DownloadQueue::startNext() {
  if (m_queue.isEmpty()) {
    return;
  }

  qDebug() << m_currentItem.url.toString();
  m_currentItem = m_queue.dequeue();

  QNetworkRequest request(m_currentItem.url);
  request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (compatible; ClipboardToolbox/1.0)");
  request.setTransferTimeout(60000);

  m_currentReply = m_networkManager->get(request);

  if (m_currentItem.startedCallback) {
    m_currentItem.startedCallback();
  }

  connect(m_currentReply, &QNetworkReply::downloadProgress, this, &DownloadQueue::onDownloadProgress);
  connect(m_currentReply, &QNetworkReply::finished, this, &DownloadQueue::onFinished);
}

void DownloadQueue::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
  qDebug() << m_currentItem.url << bytesReceived << "/" << bytesTotal;
  if (m_currentItem.progressCallback) {
    m_currentItem.progressCallback(bytesReceived, bytesTotal);
  }
}

void DownloadQueue::onFinished() {
  qDebug() << m_currentItem.url << "finished";
  QByteArray data;
  bool success = false;
  QString errorString;

  if (m_currentReply->error() == QNetworkReply::NoError) {
    data = m_currentReply->readAll();
    success = true;
  } else {
    errorString = m_currentReply->errorString();
  }

  m_currentReply->deleteLater();
  m_currentReply = nullptr;

  if (m_currentItem.finishedCallback) {
    m_currentItem.finishedCallback(data, success, errorString);
  }

  m_currentItem = DownloadItem();

  startNext();
}
