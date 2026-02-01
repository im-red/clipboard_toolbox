#include "clipboardmanager.h"

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>
#include <QStandardPaths>
#include <QUrl>

#include "settingsmanager.h"

ClipboardManager::ClipboardManager(QObject *parent)
    : QObject(parent),
      m_clipboard(QGuiApplication::clipboard()),
      m_historyManager(this),
      m_settingsManager(new SettingsManager(this)),
      m_trayIcon(new QSystemTrayIcon(this)) {
  qDebug() << "Initializing ClipboardManager";
  QIcon trayIcon(":/icon.png");
  if (trayIcon.isNull()) {
    qDebug() << "Tray icon resource not found, creating fallback pixmap";
    QPixmap pixmap(64, 64);
    pixmap.fill(QColor("#5c6bc0"));
    trayIcon = QIcon(pixmap);
  }
  m_trayIcon->setIcon(trayIcon);
  m_trayIcon->setVisible(true);

  connect(m_clipboard, &QClipboard::dataChanged, this,
          &ClipboardManager::handleClipboardChanged);
  updateFromClipboard();
}

bool ClipboardManager::hasImage() const { return m_hasImage; }

bool ClipboardManager::hasText() const { return m_hasText; }

const QMimeData *ClipboardManager::mimeData() const {
  return m_clipboard->mimeData();
}

QString ClipboardManager::latestText() const { return m_latestText; }

QImage ClipboardManager::latestImage() const { return m_latestImage; }

SettingsManager *ClipboardManager::settingsManager() const {
  return m_settingsManager;
}

HistoryManager *ClipboardManager::historyManager() { return &m_historyManager; }

QSystemTrayIcon *ClipboardManager::trayIcon() const { return m_trayIcon; }

void ClipboardManager::saveImageToFile(const QString &path) {
  qDebug() << "Attempting to save image to:" << path;
  QString localPath = normalizeLocalPath(path);
  QImage image = m_clipboard->image();
  bool success =
      !image.isNull() && !localPath.isEmpty() && image.save(localPath);

  if (success) {
    qDebug() << "Image saved successfully";
    logAction("Image saved to " + localPath, EventCategory::AutoSaveImage,
              EventLevel::Info);
  } else {
    qDebug() << "Failed to save image. Image null:" << image.isNull()
             << "Path empty:" << localPath.isEmpty();
    logAction("Failed to save image to " + localPath,
              EventCategory::AutoSaveImage, EventLevel::Error);
  }
}

void ClipboardManager::logAction(const QString &content, EventCategory category,
                                 EventLevel level) {
  qDebug() << "Logging action:" << content << "Category:" << (int)category
           << "Level:" << (int)level;
  m_historyManager.addEntry(content, category, level);

  if (m_settingsManager->notificationsEnabled() &&
      level >= m_settingsManager->notificationLevel() &&
      m_settingsManager->isCategoryNotificationEnabled(category)) {
    QString title = "Clipboard Toolbox";
    if (category == EventCategory::Copy) {
      title = "Clipboard Copied";
    } else if (category == EventCategory::AutoSaveImage) {
      title = "Auto Save Image";
    }
    showNotification(title, content, level);
  }
}

void ClipboardManager::handleClipboardChanged() {
  qDebug() << "Clipboard changed signal received";
  const QMimeData *mime = m_clipboard->mimeData();
  QString summary = summarizeMime(mime);
  if (!summary.isEmpty()) {
    QString typeTag;
    if (mime->hasText() && mime->hasImage()) {
      typeTag = "[Text + Image]";
    } else if (mime->hasText()) {
      typeTag = "[Text]";
    } else if (mime->hasImage()) {
      typeTag = "[Image]";
    }

    qDebug() << "New clipboard content detected:" << typeTag << summary;
    logAction(typeTag + " " + summary, EventCategory::Copy, EventLevel::Info);
  } else {
    qDebug() << "Clipboard content empty or unknown format";
  }
  updateFromClipboard();
}

void ClipboardManager::updateFromClipboard() {
  const QMimeData *mime = m_clipboard->mimeData();
  bool newHasImage = mime && mime->hasImage();
  bool newHasText = mime && mime->hasText();
  QString newText = newHasText ? m_clipboard->text() : QString();
  QImage newImage = newHasImage ? m_clipboard->image() : QImage();
  bool changed = false;

  qDebug() << "newText:" << newText << "newImage:" << newImage;

  if (m_hasImage != newHasImage) {
    m_hasImage = newHasImage;
    changed = true;
  }
  if (m_hasText != newHasText) {
    m_hasText = newHasText;
    changed = true;
  }
  if (m_latestText != newText) {
    m_latestText = newText;
    changed = true;
  }
  if (m_latestImage != newImage) {
    m_latestImage = newImage;
    changed = true;
  }
  if (changed) {
    qDebug() << "Internal state updated from clipboard. HasImage:" << m_hasImage
             << "HasText:" << m_hasText;
    emit clipboardChanged();
  } else {
    qDebug() << "Internal state not updated";
  }
}

QString ClipboardManager::summarizeMime(const QMimeData *mime) const {
  if (!mime) {
    return QString();
  }
  if (mime->hasText()) {
    return mime->text();
  }
  if (mime->hasImage()) {
    QImage image = qvariant_cast<QImage>(mime->imageData());
    if (image.isNull()) {
      return QString();
    }
    QString format = "Bitmap";
    for (const QString &fmt : mime->formats()) {
      if (fmt.startsWith("image/") && !fmt.contains("x-qt-")) {
        format = fmt.mid(6).toUpper();
        break;
      }
    }

    qint64 size = image.sizeInBytes();
    QString sizeStr;
    if (size >= 1024 * 1024) {
      sizeStr = QString::number(size / (1024.0 * 1024.0), 'f', 2) + " MB";
    } else if (size >= 1024) {
      sizeStr = QString::number(size / 1024.0, 'f', 2) + " KB";
    } else {
      sizeStr = QString::number(size) + " B";
    }

    return QString("%1x%2 %3 %4")
        .arg(image.width())
        .arg(image.height())
        .arg(format)
        .arg(sizeStr);
  }
  if (!mime->formats().isEmpty()) {
    return QString("Clipboard data: %1").arg(mime->formats().first());
  }
  return QString();
}

QString ClipboardManager::normalizeLocalPath(const QString &path) const {
  QUrl url(path);
  if (url.isLocalFile()) {
    return url.toLocalFile();
  }
  return path;
}

void ClipboardManager::showNotification(const QString &title,
                                        const QString &message,
                                        EventLevel level) {
  if (m_trayIcon && m_trayIcon->isVisible()) {
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information;
    switch (level) {
      case EventLevel::Info:
        icon = QSystemTrayIcon::Information;
        break;
      case EventLevel::Warning:
        icon = QSystemTrayIcon::Warning;
        break;
      case EventLevel::Error:
        icon = QSystemTrayIcon::Critical;
        break;
      default:
        icon = QSystemTrayIcon::Information;
        break;
    }

    qDebug() << "Showing notification:" << title << "-" << message;
    m_trayIcon->showMessage(title, message, icon, 3000);
  }
}
