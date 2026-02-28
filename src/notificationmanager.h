#pragma once

#include <QList>
#include <QObject>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "event.h"

class NotificationWidget : public QWidget {
  Q_OBJECT

 public:
  explicit NotificationWidget(const QString &title, const QString &message, EventLevel level,
                              QWidget *parent = nullptr);
  ~NotificationWidget();

  void slideOut();
  int notificationHeight() const { return height(); }

 signals:
  void expired();
  void slideOutFinished();

 private:
  void setupUi();
  QString levelToColor() const;

  QString m_title;
  QString m_message;
  EventLevel m_level;
  QTimer *m_timer;
  QPropertyAnimation *m_opacityAnimation;
  QPropertyAnimation *m_slideAnimation;
};

class NotificationManager : public QObject {
  Q_OBJECT

 public:
  static NotificationManager *instance();
  void showNotification(const QString &title, const QString &message, EventLevel level);

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
