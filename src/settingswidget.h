#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class ClipboardManager;

class SettingsWidget : public QWidget {
  Q_OBJECT

 public:
  explicit SettingsWidget(ClipboardManager *manager, QWidget *parent = nullptr);

 private:
  void setupUi();
  void updateNotificationToggle();
  void updateNotificationLevel();
  void updateCategoryToggles();
  void updateExitOnCloseToggle();

  ClipboardManager *m_manager = nullptr;
  QCheckBox *m_notificationToggle = nullptr;
  QComboBox *m_levelCombo = nullptr;
  QCheckBox *m_copyNotifyToggle = nullptr;
  QCheckBox *m_autoSaveNotifyToggle = nullptr;
  QCheckBox *m_exitOnCloseToggle = nullptr;
};
