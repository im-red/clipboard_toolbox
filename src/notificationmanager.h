#pragma once

#include <QList>
#include <QObject>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "event.h"

class QProgressBar;

class Notification : public QObject {
  Q_OBJECT

 public:
  static constexpr int DEFAULT_DURATION = 5000;

  explicit Notification(const QString &title, const QString &message, EventLevel level, QObject *parent = nullptr);
  ~Notification();

  QString title() const { return m_title; }
  QString message() const { return m_message; }
  EventLevel level() const { return m_level; }

  bool hasProgress() const { return m_hasProgress; }
  void setHasProgress(bool has);

  int progress() const { return m_progress; }
  void setProgress(int progress);

  bool isError() const { return m_isError; }
  void setIsError(bool isError);

  void startExpiration(int durationMs = -1);
  void pauseExpiration();
  void resumeExpiration();
  void cancelExpiration();
  void expireNow();

 signals:
  void expired();
  void progressChanged(int progress);
  void hasProgressChanged(bool hasProgress);
  void isErrorChanged(bool isError);
  void dataChanged();

 private:
  QString m_title;
  QString m_message;
  EventLevel m_level;
  bool m_hasProgress = false;
  int m_progress = 0;
  bool m_isError = false;
  QTimer *m_expirationTimer = nullptr;
  bool m_expirationPaused = false;
  int m_remainingExpirationTime = 0;
};

class NotificationWidget : public QWidget {
  Q_OBJECT

 public:
  explicit NotificationWidget(Notification *notification, QWidget *parent = nullptr);
  ~NotificationWidget();

  void slideOut();
  int notificationHeight() const { return height(); }
  Notification *notification() const { return m_notification; }

 signals:
  void slideOutFinished();

 private:
  void setupUi();
  QString levelToColor() const;
  void updateProgressBar();
  void updateStyle();

  Notification *m_notification;
  QProgressBar *m_progressBar = nullptr;
  QPropertyAnimation *m_opacityAnimation;
  QPropertyAnimation *m_slideAnimation;
};

class NotificationManager : public QObject {
  Q_OBJECT

 public:
  static NotificationManager *instance();

  Notification *showNotification(const QString &title, const QString &message, EventLevel level);
  Notification *showProgressNotification(const QString &title, const QString &message,
                                         EventLevel level = EventLevel::Info);

 private:
  explicit NotificationManager(QObject *parent = nullptr);
  ~NotificationManager();

  void ensureContainerVisible();
  void repositionContainer();
  void repositionContainerAnimated(int removedHeight);
  void startSlideOut(NotificationWidget *widget);
  void removeNotification(NotificationWidget *widget);

  static NotificationManager *s_instance;
  QWidget *m_container;
  QVBoxLayout *m_layout;
  QList<NotificationWidget *> m_notifications;
  static constexpr int MAX_NOTIFICATIONS = 5;
  static constexpr int NOTIFICATION_DURATION = 5000;
};
