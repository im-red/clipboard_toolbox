#pragma once

#include <QObject>
#include <QSettings>

#include "historymanager.h"

class SettingsManager : public QObject {
  Q_OBJECT

 public:
  explicit SettingsManager(QObject *parent = nullptr);

  bool notificationsEnabled() const;
  void setNotificationsEnabled(bool enabled);

  EventLevel notificationLevel() const;
  void setNotificationLevel(EventLevel level);

  bool isCategoryNotificationEnabled(EventCategory category) const;
  void setCategoryNotificationEnabled(EventCategory category, bool enabled);

  bool exitOnClose() const;
  void setExitOnClose(bool exit);

  bool autoSaveEnabled() const;
  void setAutoSaveEnabled(bool enabled);

  QString autoSavePath() const;
  void setAutoSavePath(const QString &path);

  QStringList recentAutoSavePaths() const;                // Added
  void setRecentAutoSavePaths(const QStringList &paths);  // Added

  int autoSaveMaxSizeMB() const;
  void setAutoSaveMaxSizeMB(int sizeMB);

 signals:
  void notificationsEnabledChanged(bool enabled);
  void notificationLevelChanged(EventLevel level);
  void categoryNotificationEnabledChanged(EventCategory category, bool enabled);
  void exitOnCloseChanged(bool exit);
  void autoSaveEnabledChanged(bool enabled);
  void autoSavePathChanged(const QString &path);
  void recentAutoSavePathsChanged(const QStringList &paths);  // Added
  void autoSaveMaxSizeMBChanged(int sizeMB);

 private:
  QSettings createSettings() const;
  QString getCategoryKey(EventCategory category) const;

  bool m_notificationsEnabled = true;
  EventLevel m_notificationLevel = EventLevel::Info;
  bool m_copyNotificationEnabled = true;
  bool m_autoSaveNotificationEnabled = true;
  bool m_exitOnClose = false;
  bool m_autoSaveEnabled = false;
  QString m_autoSavePath;
  QStringList m_recentAutoSavePaths;  // Added
  int m_autoSaveMaxSizeMB = 30;
};
