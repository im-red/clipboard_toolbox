#include "notificationmanager.h"

#include <QAbstractAnimation>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPainter>
#include <QScreen>
#include <QSizePolicy>

NotificationManager *NotificationManager::s_instance = nullptr;

NotificationWidget::NotificationWidget(const QString &title, const QString &message, EventLevel level, QWidget *parent)
    : QWidget(parent), m_title(title), m_message(message), m_level(level) {
  setupUi();

  m_timer = new QTimer(this);
  m_timer->setSingleShot(true);
  connect(m_timer, &QTimer::timeout, this, &NotificationWidget::expired);

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

  auto *titleLabel = new QLabel(m_title, this);
  titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
  mainLayout->addWidget(titleLabel);

  auto *messageLabel = new QLabel(m_message, this);
  messageLabel->setWordWrap(true);
  messageLabel->setStyleSheet("font-size: 11px;");
  mainLayout->addWidget(messageLabel);

  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  adjustSize();

  QString borderColor = levelToColor();
  setStyleSheet(QString("NotificationWidget {"
                        "  background-color: #2d2d2d;"
                        "  border: 2px solid %1;"
                        "  border-radius: 6px;"
                        "}"
                        "QLabel { color: #ffffff; background: transparent; }")
                    .arg(borderColor));

  setAttribute(Qt::WA_StyledBackground, true);
}

QString NotificationWidget::levelToColor() const {
  switch (m_level) {
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

void NotificationManager::showNotification(const QString &title, const QString &message, EventLevel level) {
  while (m_notifications.size() >= MAX_NOTIFICATIONS) {
    auto *oldest = m_notifications.last();
    startSlideOut(oldest);
  }

  auto *widget = new NotificationWidget(title, message, level, m_container);
  connect(widget, &NotificationWidget::expired, this, [this, widget]() { startSlideOut(widget); });

  m_notifications.prepend(widget);
  m_layout->insertWidget(1, widget);

  widget->show();
  ensureContainerVisible();
  repositionContainer();

  QTimer::singleShot(NOTIFICATION_DURATION, widget, &NotificationWidget::expired);
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
  widget->deleteLater();

  if (m_notifications.isEmpty()) {
    m_container->hide();
  }
}
