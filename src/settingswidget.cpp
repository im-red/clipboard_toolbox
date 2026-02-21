#include "settingswidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "clipboardmanager.h"
#include "settingsmanager.h"

SettingsWidget::SettingsWidget(ClipboardManager *manager, QWidget *parent) : QWidget(parent), m_manager(manager) {
  setupUi();
  updateNotificationToggle();
  updateNotificationLevel();
  updateCategoryToggles();
  updateExitOnCloseToggle();

  connect(m_manager->settingsManager(), &SettingsManager::notificationsEnabledChanged, this,
          &SettingsWidget::updateNotificationToggle);
  connect(m_manager->settingsManager(), &SettingsManager::notificationLevelChanged, this,
          &SettingsWidget::updateNotificationLevel);
  connect(m_manager->settingsManager(), &SettingsManager::categoryNotificationEnabledChanged, this,
          &SettingsWidget::updateCategoryToggles);
  connect(m_manager->settingsManager(), &SettingsManager::exitOnCloseChanged, this,
          &SettingsWidget::updateExitOnCloseToggle);
}

void SettingsWidget::setupUi() {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(12);

  auto *generalGroup = new QGroupBox("General");
  auto *generalLayout = new QHBoxLayout(generalGroup);
  m_exitOnCloseToggle = new QCheckBox("Exit application when main window is closed");
  generalLayout->addWidget(m_exitOnCloseToggle);
  generalLayout->addStretch();
  layout->addWidget(generalGroup);

  connect(m_exitOnCloseToggle, &QCheckBox::toggled, this,
          [this](bool checked) { m_manager->settingsManager()->setExitOnClose(checked); });

  auto *notificationGroup = new QGroupBox("Notifications");
  auto *notificationLayout = new QVBoxLayout(notificationGroup);

  // Master switch
  m_notificationToggle = new QCheckBox("Enable System Notifications");
  notificationLayout->addWidget(m_notificationToggle);

  // Level selector
  auto *levelLayout = new QHBoxLayout();
  levelLayout->addWidget(new QLabel("Minimum Level:"));
  m_levelCombo = new QComboBox();
  m_levelCombo->addItem("Info", (int)EventLevel::Info);
  m_levelCombo->addItem("Warning", (int)EventLevel::Warning);
  m_levelCombo->addItem("Error", (int)EventLevel::Error);
  levelLayout->addWidget(m_levelCombo);
  levelLayout->addStretch();
  notificationLayout->addLayout(levelLayout);

  // Categories
  m_copyNotifyToggle = new QCheckBox("Notify on Copy");
  notificationLayout->addWidget(m_copyNotifyToggle);

  m_autoSaveNotifyToggle = new QCheckBox("Notify on AutoSave Image");
  notificationLayout->addWidget(m_autoSaveNotifyToggle);

  layout->addWidget(notificationGroup);

  connect(m_notificationToggle, &QCheckBox::toggled, this,
          [this](bool checked) { m_manager->settingsManager()->setNotificationsEnabled(checked); });

  connect(m_levelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
    EventLevel level = (EventLevel)m_levelCombo->itemData(index).toInt();
    m_manager->settingsManager()->setNotificationLevel(level);
  });

  connect(m_copyNotifyToggle, &QCheckBox::toggled, this, [this](bool checked) {
    m_manager->settingsManager()->setCategoryNotificationEnabled(EventCategory::Copy, checked);
  });

  connect(m_autoSaveNotifyToggle, &QCheckBox::toggled, this, [this](bool checked) {
    m_manager->settingsManager()->setCategoryNotificationEnabled(EventCategory::AutoSaveImage, checked);
  });

  layout->addStretch();
}

void SettingsWidget::updateNotificationToggle() {
  bool enabled = m_manager->settingsManager()->notificationsEnabled();
  m_notificationToggle->blockSignals(true);
  m_notificationToggle->setChecked(enabled);
  m_notificationToggle->blockSignals(false);

  // Enable/disable sub-controls based on master switch
  m_levelCombo->setEnabled(enabled);
  m_copyNotifyToggle->setEnabled(enabled);
  m_autoSaveNotifyToggle->setEnabled(enabled);
}

void SettingsWidget::updateNotificationLevel() {
  EventLevel level = m_manager->settingsManager()->notificationLevel();
  m_levelCombo->blockSignals(true);
  int index = m_levelCombo->findData((int)level);
  if (index != -1) {
    m_levelCombo->setCurrentIndex(index);
  }
  m_levelCombo->blockSignals(false);
}

void SettingsWidget::updateCategoryToggles() {
  m_copyNotifyToggle->blockSignals(true);
  m_copyNotifyToggle->setChecked(m_manager->settingsManager()->isCategoryNotificationEnabled(EventCategory::Copy));
  m_copyNotifyToggle->blockSignals(false);

  m_autoSaveNotifyToggle->blockSignals(true);
  m_autoSaveNotifyToggle->setChecked(
      m_manager->settingsManager()->isCategoryNotificationEnabled(EventCategory::AutoSaveImage));
  m_autoSaveNotifyToggle->blockSignals(false);
}

void SettingsWidget::updateExitOnCloseToggle() {
  bool exit = m_manager->settingsManager()->exitOnClose();
  m_exitOnCloseToggle->blockSignals(true);
  m_exitOnCloseToggle->setChecked(exit);
  m_exitOnCloseToggle->blockSignals(false);
}
