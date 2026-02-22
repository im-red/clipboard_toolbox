#pragma once

#include <QByteArray>
#include <QSet>
#include <QWidget>

class ClipboardManager;
class QCheckBox;
class QComboBox;  // Added QComboBox
class QLineEdit;
class QPushButton;
class QSpinBox;
class QLabel;
class QNetworkAccessManager;
class QProgressBar;
class QFile;

class AutoSaveWidget : public QWidget {
  Q_OBJECT

 public:
  explicit AutoSaveWidget(ClipboardManager *manager, QWidget *parent = nullptr);
  ~AutoSaveWidget();

 private slots:
  void onToggleChanged(int state);
  void onMaxSizeChanged(int value);
  void onBrowseClicked();
  void onOpenDirClicked();         // Added
  void onPathSelected(int index);  // Added
  void onCleanClicked();
  void onRebuildClicked();
  void onClipboardChanged();

 private:
  void setupUi();
  bool saveImage(QFile &file, const QString &originalName = QString(), const QString &source = QString());
  bool saveImage(const QImage &image);
  void loadSettings();
  void saveSettings();
  void loadChecksums();
  void appendChecksum(const QString &filename, const QByteArray &checksum);
  QByteArray calculateChecksum(QIODevice *device);
  bool processTextContent();
  bool processImageContent();
  bool handleRemoteUrl(const QUrl &url, const QImage &fallbackImage = QImage());
  bool handleLocalPath(const QString &path);
  void updateRecentPaths(const QString &path);  // Helper
  void populatePathCombo();                     // Helper

  ClipboardManager *m_manager = nullptr;
  QCheckBox *m_enableCheckBox = nullptr;
  QLabel *m_pathLabel = nullptr;
  QComboBox *m_pathCombo = nullptr;  // Replaced m_pathEdit
  QPushButton *m_browseButton = nullptr;
  QPushButton *m_openDirButton = nullptr;  // Added
  QPushButton *m_cleanButton = nullptr;
  QPushButton *m_rebuildButton = nullptr;
  QLabel *m_maxSizeLabel = nullptr;
  QSpinBox *m_maxSizeSpinBox = nullptr;
  QProgressBar *m_progressBar = nullptr;

  QString m_targetDir;
  QStringList m_recentPaths;  // Added
  bool m_isEnabled = false;
  bool m_isRebuilding = false;
  int m_maxSizeMB = 30;
  QSet<QByteArray> m_seenChecksums;
  QNetworkAccessManager *m_networkManager = nullptr;
};
