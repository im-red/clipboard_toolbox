#pragma once

#include <QClipboard>
#include <QDateTime>
#include <QImage>
#include <QMimeData>
#include <QObject>
#include <QSettings>
#include <QSystemTrayIcon>

#include "historymanager.h"

class SettingsManager;

class ClipboardManager : public QObject {
  Q_OBJECT

 public:
  explicit ClipboardManager(QObject *parent = nullptr);

  bool hasImage() const;
  bool hasText() const;
  const QMimeData *mimeData() const;
  QString latestText() const;
  QImage latestImage() const;
  SettingsManager *settingsManager() const;
  HistoryManager *historyManager();
  QSystemTrayIcon *trayIcon() const;

  void saveImageToFile(const QString &path);
  void logAction(const QString &content, EventCategory category, EventLevel level);

 signals:
  void clipboardChanged();

 private:
  void handleClipboardChanged();
  void updateFromClipboard();
  QString summarizeMime(const QMimeData *mime) const;
  QString normalizeLocalPath(const QString &path) const;
  void showNotification(const QString &title, const QString &message, EventLevel level);

  QClipboard *m_clipboard = nullptr;
  bool m_hasImage = false;
  bool m_hasText = false;
  QString m_latestText;
  QImage m_latestImage;
  HistoryManager m_historyManager;
  SettingsManager *m_settingsManager = nullptr;
  QSystemTrayIcon *m_trayIcon = nullptr;
};
