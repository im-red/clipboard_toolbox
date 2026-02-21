#include "settingsmanager.h"

#include <QDir>
#include <QStandardPaths>

SettingsManager::SettingsManager(QObject *parent) : QObject(parent) {
  QSettings settings = createSettings();
  m_notificationsEnabled = settings.value("notificationsEnabled", true).toBool();
  m_exitOnClose = settings.value("exitOnClose", false).toBool();
  m_autoSaveEnabled = settings.value("autoSaveEnabled", false).toBool();
  m_autoSavePath = settings.value("autoSavePath", "").toString();
  m_recentAutoSavePaths = settings.value("recentAutoSavePaths").toStringList();  // Added
  m_autoSaveMaxSizeMB = settings.value("autoSaveMaxSizeMB", 30).toInt();

  int levelInt = settings.value("notificationLevel", (int)EventLevel::Info).toInt();
  m_notificationLevel = static_cast<EventLevel>(levelInt);

  // Initialize categories with defaults
  m_copyNotificationEnabled = settings.value("categoryNotification_Copy", true).toBool();
  m_autoSaveNotificationEnabled = settings.value("categoryNotification_AutoSaveImage", true).toBool();

  qDebug() << "Settings loaded. Copy:" << m_copyNotificationEnabled << "AutoSave:" << m_autoSaveNotificationEnabled
           << "from" << settings.fileName();
}

EventLevel SettingsManager::notificationLevel() const { return m_notificationLevel; }

void SettingsManager::setNotificationLevel(EventLevel level) {
  if (m_notificationLevel == level) {
    return;
  }
  m_notificationLevel = level;
  QSettings settings = createSettings();
  settings.setValue("notificationLevel", (int)level);
  emit notificationLevelChanged(level);
}

bool SettingsManager::isCategoryNotificationEnabled(EventCategory category) const {
  if (category == EventCategory::Copy) {
    return m_copyNotificationEnabled;
  } else if (category == EventCategory::AutoSaveImage) {
    return m_autoSaveNotificationEnabled;
  }
  return true;
}

void SettingsManager::setCategoryNotificationEnabled(EventCategory category, bool enabled) {
  bool *target = nullptr;
  if (category == EventCategory::Copy) {
    target = &m_copyNotificationEnabled;
  } else if (category == EventCategory::AutoSaveImage) {
    target = &m_autoSaveNotificationEnabled;
  }

  if (!target || *target == enabled) {
    return;
  }
  *target = enabled;

  QSettings settings = createSettings();
  QString key = "categoryNotification_" + HistoryManager::categoryToString(category);
  settings.setValue(key, enabled);
  settings.sync();
  if (settings.status() != QSettings::NoError) {
    qDebug() << "Error saving settings:" << settings.status();
  }
  qDebug() << "Saved category notification:" << key << enabled << "to" << settings.fileName();

  emit categoryNotificationEnabledChanged(category, enabled);
}

bool SettingsManager::notificationsEnabled() const { return m_notificationsEnabled; }

void SettingsManager::setNotificationsEnabled(bool enabled) {
  if (m_notificationsEnabled == enabled) {
    return;
  }
  m_notificationsEnabled = enabled;
  QSettings settings = createSettings();
  settings.setValue("notificationsEnabled", enabled);
  emit notificationsEnabledChanged(enabled);
}

bool SettingsManager::exitOnClose() const { return m_exitOnClose; }

void SettingsManager::setExitOnClose(bool exit) {
  if (m_exitOnClose == exit) {
    return;
  }
  m_exitOnClose = exit;
  QSettings settings = createSettings();
  settings.setValue("exitOnClose", exit);
  emit exitOnCloseChanged(exit);
}

bool SettingsManager::autoSaveEnabled() const { return m_autoSaveEnabled; }

void SettingsManager::setAutoSaveEnabled(bool enabled) {
  if (m_autoSaveEnabled == enabled) {
    return;
  }
  m_autoSaveEnabled = enabled;
  QSettings settings = createSettings();
  settings.setValue("autoSaveEnabled", enabled);
  emit autoSaveEnabledChanged(enabled);
}

QString SettingsManager::autoSavePath() const { return m_autoSavePath; }

void SettingsManager::setAutoSavePath(const QString &path) {
  if (m_autoSavePath == path) {
    return;
  }
  m_autoSavePath = path;
  QSettings settings = createSettings();
  settings.setValue("autoSavePath", path);
  emit autoSavePathChanged(path);
}

QStringList SettingsManager::recentAutoSavePaths() const { return m_recentAutoSavePaths; }

void SettingsManager::setRecentAutoSavePaths(const QStringList &paths) {
  if (m_recentAutoSavePaths == paths) {
    return;
  }
  m_recentAutoSavePaths = paths;
  QSettings settings = createSettings();
  settings.setValue("recentAutoSavePaths", paths);
  emit recentAutoSavePathsChanged(paths);
}

int SettingsManager::autoSaveMaxSizeMB() const { return m_autoSaveMaxSizeMB; }

void SettingsManager::setAutoSaveMaxSizeMB(int sizeMB) {
  if (m_autoSaveMaxSizeMB == sizeMB) {
    return;
  }
  m_autoSaveMaxSizeMB = sizeMB;
  QSettings settings = createSettings();
  settings.setValue("autoSaveMaxSizeMB", sizeMB);
  emit autoSaveMaxSizeMBChanged(sizeMB);
}

QSettings SettingsManager::createSettings() const {
  QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(dataLocation);
  if (!dir.exists()) {
    dir.mkpath(".");
  }
  QString configPath = dir.filePath("settings.ini");
  return QSettings(configPath, QSettings::IniFormat);
}

QString SettingsManager::getCategoryKey(EventCategory category) const {
  return "categoryNotification_" + HistoryManager::categoryToString(category);
}
