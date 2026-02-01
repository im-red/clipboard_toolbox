#pragma once

#include <QMainWindow>

#include "clipboardmanager.h"

class QCheckBox;
class QGroupBox;
class QLabel;
class QListView;
class QTextEdit;
class QTableWidget;
class SettingsWidget;
class HistoryWidget;
class ContentWidget;
class MimeWidget;
class AutoSaveWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = nullptr);

 private:
  void setupUi();

  ClipboardManager m_clipboardManager;
  SettingsWidget *m_settingsWidget = nullptr;
  HistoryWidget *m_historyWidget = nullptr;
  ContentWidget *m_contentWidget = nullptr;
  MimeWidget *m_mimeWidget = nullptr;
  AutoSaveWidget *m_autoSaveWidget = nullptr;

 protected:
  void resizeEvent(QResizeEvent *event) override;
  void closeEvent(QCloseEvent *event) override;
};
