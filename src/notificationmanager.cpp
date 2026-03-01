#include "notificationmanager.h"

#include <QAbstractAnimation>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPainter>
#include <QProgressBar>
#include <QScreen>
#include <QSizePolicy>

NotificationManager *NotificationManager::s_instance = nullptr;

Notification::Notification(const QString &title, const QString &message, EventLevel level, QObject *parent)
    : QObject(parent), m_title(title), m_message(message), m_level(level) {
  m_expirationTimer = new QTimer(this);
  m_expirationTimer->setSingleShot(true);
  connect(m_expirationTimer, &QTimer::timeout, this, &Notification::expired);
}

Notification::~Notification() {}

void Notification::setHasProgress(bool has) {
  if (m_hasProgress != has) {
    m_hasProgress = has;
    emit hasProgressChanged(has);
    emit dataChanged();
  }
}

void Notification::setProgress(int progress) {
  if (m_progress != progress) {
    m_progress = progress;
    emit progressChanged(progress);
    emit dataChanged();
  }
}

void Notification::setIsError(bool isError) {
  if (m_isError != isError) {
    m_isError = isError;
    emit isErrorChanged(isError);
    emit dataChanged();
  }
}

void Notification::startExpiration(int durationMs) {
  if (durationMs < 0) {
    durationMs = Notification::DEFAULT_DURATION;
  }
  m_remainingExpirationTime = durationMs;
  m_expirationPaused = false;
  m_expirationTimer->start(durationMs);
}

void Notification::pauseExpiration() {
  if (m_expirationTimer->isActive() && !m_expirationPaused) {
    m_remainingExpirationTime = m_expirationTimer->remainingTime();
    m_expirationTimer->stop();
    m_expirationPaused = true;
  }
}

void Notification::resumeExpiration() {
  if (m_expirationPaused && m_remainingExpirationTime > 0) {
    m_expirationTimer->start(m_remainingExpirationTime);
    m_expirationPaused = false;
  }
}

void Notification::cancelExpiration() {
  m_expirationTimer->stop();
  m_remainingExpirationTime = 0;
  m_expirationPaused = false;
}

void Notification::expireNow() {
  m_expirationTimer->stop();
  emit expired();
}

NotificationWidget::NotificationWidget(Notification *notification, QWidget *parent)
    : QWidget(parent), m_notification(notification) {
  setupUi();

  connect(m_notification, &Notification::dataChanged, this, [this]() {
    updateProgressBar();
    updateStyle();
  });

  m_opacityAnimation = new QPropertyAnimation(this, "windowOpacity", this);
  m_opacityAnimation->setDuration(200);
  m_opacityAnimation->setStartValue(0.0);
  m_opacityAnimation->setEndValue(1.0);
  m_opacityAnimation->start();
}

NotificationWidget::~NotificationWidget() {}

void NotificationWidget::slideOut() {
  m_slideAnimation = new QPropertyAnimation(this, "geometry", this);
  m_slideAnimation->setDuration(200);

  QRect currentGeo = geometry();
  QRect targetGeo = QRect(currentGeo.right(), currentGeo.top(), currentGeo.width(), currentGeo.height());

  m_slideAnimation->setStartValue(currentGeo);
  m_slideAnimation->setEndValue(targetGeo);

  connect(m_slideAnimation, &QPropertyAnimation::finished, this, [this]() { emit slideOutFinished(); });

  m_slideAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void NotificationWidget::setupUi() {
  setFixedWidth(300);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(12, 8, 12, 8);
  mainLayout->setSpacing(4);

  auto *titleLabel = new QLabel(m_notification->title(), this);
  titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
  mainLayout->addWidget(titleLabel);

  auto *messageLabel = new QLabel(m_notification->message(), this);
  messageLabel->setWordWrap(true);
  messageLabel->setStyleSheet("font-size: 11px;");
  mainLayout->addWidget(messageLabel);

  if (m_notification->hasProgress()) {
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(m_notification->progress());
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedHeight(16);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #3d3d3d;"
        "  border: 1px solid #555;"
        "  border-radius: 3px;"
        "  text-align: center;"
        "  color: white;"
        "  font-size: 10px;"
        "}"
        "QProgressBar::chunk {"
        "  background-color: #4a90d9;"
        "  border-radius: 2px;"
        "}");
    mainLayout->addWidget(m_progressBar);
  }

  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  adjustSize();

  updateStyle();
  setAttribute(Qt::WA_StyledBackground, true);
}

QString NotificationWidget::levelToColor() const {
  if (m_notification->isError()) {
    return "#d94a4a";
  }
  switch (m_notification->level()) {
    case EventLevel::Info:
      return "#4a90d9";
    case EventLevel::Warning:
      return "#d9a74a";
    case EventLevel::Error:
      return "#d94a4a";
    default:
      return "#4a90d9";
  }
}

void NotificationWidget::updateProgressBar() {
  if (m_progressBar && m_notification->hasProgress()) {
    m_progressBar->setValue(m_notification->progress());
  }
}

void NotificationWidget::updateStyle() {
  QString borderColor = levelToColor();
  setStyleSheet(QString("NotificationWidget {"
                        "  background-color: #2d2d2d;"
                        "  border: 2px solid %1;"
                        "  border-radius: 6px;"
                        "}"
                        "QLabel { color: #ffffff; background: transparent; }")
                    .arg(borderColor));
}

NotificationManager *NotificationManager::instance() {
  if (!s_instance) {
    s_instance = new NotificationManager();
  }
  return s_instance;
}

NotificationManager::NotificationManager(QObject *parent) : QObject(parent) {
  m_container = new QWidget(nullptr);
  m_container->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  m_container->setAttribute(Qt::WA_TranslucentBackground);
  m_container->setAttribute(Qt::WA_ShowWithoutActivating);
  m_container->setFixedSize(320, 500);

  m_layout = new QVBoxLayout(m_container);
  m_layout->setContentsMargins(10, 10, 10, 10);
  m_layout->setSpacing(8);
  m_layout->addStretch();
}

NotificationManager::~NotificationManager() {
  if (m_container) {
    delete m_container;
  }
}

Notification *NotificationManager::showNotification(const QString &title, const QString &message, EventLevel level) {
  while (m_notifications.size() >= MAX_NOTIFICATIONS) {
    auto *oldest = m_notifications.last();
    oldest->notification()->expireNow();
  }

  auto *notification = new Notification(title, message, level, this);
  auto *widget = new NotificationWidget(notification, m_container);

  connect(notification, &Notification::expired, this, [this, widget]() { startSlideOut(widget); });

  m_notifications.prepend(widget);
  m_layout->insertWidget(1, widget);

  widget->show();
  ensureContainerVisible();
  repositionContainer();

  notification->startExpiration();

  return notification;
}

Notification *NotificationManager::showProgressNotification(const QString &title, const QString &message,
                                                            EventLevel level) {
  while (m_notifications.size() >= MAX_NOTIFICATIONS) {
    auto *oldest = m_notifications.last();
    oldest->notification()->expireNow();
  }

  auto *notification = new Notification(title, message, level, this);
  notification->setHasProgress(true);

  auto *widget = new NotificationWidget(notification, m_container);

  connect(notification, &Notification::expired, this, [this, widget]() { startSlideOut(widget); });

  m_notifications.prepend(widget);
  m_layout->insertWidget(1, widget);

  widget->show();
  ensureContainerVisible();
  repositionContainer();

  return notification;
}

void NotificationManager::ensureContainerVisible() {
  if (!m_container->isVisible()) {
    m_container->show();
  }
}

void NotificationManager::repositionContainer() {
  QScreen *screen = QApplication::primaryScreen();
  if (!screen) return;

  QRect screenGeometry = screen->availableGeometry();
  int x = screenGeometry.right() - m_container->width() - 10;
  int y = screenGeometry.bottom() - m_container->height() - 10;

  m_container->move(x, y);
}

void NotificationManager::repositionContainerAnimated(int removedHeight) {
  Q_UNUSED(removedHeight);

  QScreen *screen = QApplication::primaryScreen();
  if (!screen) return;

  QRect screenGeometry = screen->availableGeometry();

  int x = screenGeometry.right() - m_container->width() - 10;
  int targetY = screenGeometry.bottom() - m_container->height() - 10;

  auto *animation = new QPropertyAnimation(m_container, "pos", this);
  animation->setDuration(200);
  animation->setStartValue(m_container->pos());
  animation->setEndValue(QPoint(x, targetY));

  animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void NotificationManager::startSlideOut(NotificationWidget *widget) {
  if (!widget || !m_notifications.contains(widget)) return;

  int removedHeight = widget->notificationHeight();

  connect(widget, &NotificationWidget::slideOutFinished, this, [this, widget, removedHeight]() {
    removeNotification(widget);
    repositionContainerAnimated(removedHeight);
  });

  widget->slideOut();
}

void NotificationManager::removeNotification(NotificationWidget *widget) {
  if (!widget || !m_notifications.contains(widget)) return;

  m_notifications.removeOne(widget);
  m_layout->removeWidget(widget);

  if (widget->notification()) {
    widget->notification()->deleteLater();
  }
  widget->deleteLater();

  if (m_notifications.isEmpty()) {
    m_container->hide();
  }
}
