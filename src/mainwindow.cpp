#include "mainwindow.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMetaEnum>
#include <QPixmap>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QUrl>
#include <QVBoxLayout>

#include "autosavewidget.h"
#include "contentwidget.h"
#include "historymanager.h"
#include "historywidget.h"
#include "mimewidget.h"
#include "settingsmanager.h"
#include "settingswidget.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUi();

  // Tray Icon Setup
  QSystemTrayIcon *tray = m_clipboardManager.trayIcon();
  if (tray) {
    auto *trayMenu = new QMenu(this);
    auto *showAction = trayMenu->addAction("Show/Hide");
    auto *exitAction = trayMenu->addAction("Exit");

    connect(showAction, &QAction::triggered, this, [this]() {
      if (isVisible()) {
        hide();
      } else {
        show();
        activateWindow();
      }
    });

    connect(exitAction, &QAction::triggered,
            []() { QCoreApplication::quit(); });

    tray->setContextMenu(trayMenu);

    connect(tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
              if (reason == QSystemTrayIcon::Trigger) {
                if (isVisible()) {
                  hide();
                } else {
                  show();
                  activateWindow();
                }
              }
            });
  }
}

void MainWindow::setupUi() {
  setWindowTitle("Clipboard Toolbox");
  resize(900, 720);

  auto *menuBar = this->menuBar();
  auto *fileMenu = menuBar->addMenu("File");
  auto *openDataFolderAction = fileMenu->addAction("Open data folder");
  connect(openDataFolderAction, &QAction::triggered, this, []() {
    QString path =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) {
      dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
  });

  fileMenu->addSeparator();
  auto *exitAction = fileMenu->addAction("Exit");
  connect(exitAction, &QAction::triggered, []() { QCoreApplication::quit(); });

  auto *splitter = new QSplitter(Qt::Vertical, this);
  splitter->setChildrenCollapsible(false);
  setCentralWidget(splitter);

  auto *tabs = new QTabWidget();
  splitter->addWidget(tabs);

  m_contentWidget = new ContentWidget(&m_clipboardManager, this);
  tabs->addTab(m_contentWidget, "Content");

  m_mimeWidget = new MimeWidget(&m_clipboardManager, this);
  tabs->addTab(m_mimeWidget, "MIME Data");

  m_autoSaveWidget = new AutoSaveWidget(&m_clipboardManager, this);
  tabs->addTab(m_autoSaveWidget, "Auto Save Image");

  m_settingsWidget = new SettingsWidget(&m_clipboardManager, this);
  tabs->addTab(m_settingsWidget, "Settings");

  m_historyWidget = new HistoryWidget(&m_clipboardManager, this);
  splitter->addWidget(m_historyWidget);

  // Set initial splitter sizes (e.g., 60% top, 40% bottom)
  splitter->setSizes({432, 288});
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (!event->spontaneous() ||
      m_clipboardManager.settingsManager()->exitOnClose()) {
    event->accept();
  } else {
    event->ignore();
    hide();
    if (auto *tray = m_clipboardManager.trayIcon()) {
      tray->showMessage("Clipboard Toolbox",
                        "Application is running in the background.",
                        QSystemTrayIcon::Information, 2000);
    }
  }
}
