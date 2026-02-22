#include "autosavewidget.h"

#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>  // Added
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>  // Added
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>
#include <QVBoxLayout>

#include "clipboardmanager.h"
#include "settingsmanager.h"

static const QString kChecksumFileName = "checksums.txt";
static const QString kClearRecentPaths = "<Clear Recent Paths>";

AutoSaveWidget::AutoSaveWidget(ClipboardManager* manager, QWidget* parent) : QWidget(parent), m_manager(manager) {
  m_networkManager = new QNetworkAccessManager(this);
  setupUi();
  loadSettings();
  loadChecksums();

  connect(m_manager, &ClipboardManager::clipboardChanged, this, &AutoSaveWidget::onClipboardChanged);
}

AutoSaveWidget::~AutoSaveWidget() {}

void AutoSaveWidget::setupUi() {
  auto layout = new QVBoxLayout(this);

  // Row 1: Enable Toggle
  auto toggleLayout = new QHBoxLayout();
  m_enableCheckBox = new QCheckBox("Enable Auto Save", this);
  toggleLayout->addWidget(m_enableCheckBox);
  toggleLayout->addStretch();
  layout->addLayout(toggleLayout);

  // Row 2: Path Selection
  auto pathLayout = new QHBoxLayout();
  m_pathLabel = new QLabel("Save Directory:", this);
  pathLayout->addWidget(m_pathLabel);

  m_pathCombo = new QComboBox(this);
  m_pathCombo->setEditable(false);
  m_pathCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  pathLayout->addWidget(m_pathCombo);

  m_browseButton = new QPushButton("Browse...", this);
  pathLayout->addWidget(m_browseButton);

  m_openDirButton = new QPushButton("Open", this);
  pathLayout->addWidget(m_openDirButton);

  m_cleanButton = new QPushButton("Clean Checksums", this);
  pathLayout->addWidget(m_cleanButton);

  m_rebuildButton = new QPushButton("Rebuild Checksums", this);
  pathLayout->addWidget(m_rebuildButton);

  layout->addLayout(pathLayout);

  // Row 3: Max Size
  auto sizeLayout = new QHBoxLayout();
  m_maxSizeLabel = new QLabel("Max Size (MB):", this);
  sizeLayout->addWidget(m_maxSizeLabel);
  m_maxSizeSpinBox = new QSpinBox(this);
  m_maxSizeSpinBox->setRange(1, 1000);  // 1MB to 1000MB
  m_maxSizeSpinBox->setValue(30);
  m_maxSizeSpinBox->setMinimumWidth(120);  // UI Polish: Make it wider
  sizeLayout->addWidget(m_maxSizeSpinBox);
  sizeLayout->addStretch();
  layout->addLayout(sizeLayout);

  // Row 4: Progress Bar (Hidden by default)
  m_progressBar = new QProgressBar(this);
  m_progressBar->setVisible(false);
  layout->addWidget(m_progressBar);

  // UI Polish: Push everything to the top
  layout->addStretch();

  connect(m_enableCheckBox, &QCheckBox::stateChanged, this, &AutoSaveWidget::onToggleChanged);
  connect(m_maxSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &AutoSaveWidget::onMaxSizeChanged);
  connect(m_browseButton, &QPushButton::clicked, this, &AutoSaveWidget::onBrowseClicked);
  connect(m_openDirButton, &QPushButton::clicked, this, &AutoSaveWidget::onOpenDirClicked);
  connect(m_pathCombo, QOverload<int>::of(&QComboBox::activated), this, &AutoSaveWidget::onPathSelected);
  connect(m_cleanButton, &QPushButton::clicked, this, &AutoSaveWidget::onCleanClicked);
  connect(m_rebuildButton, &QPushButton::clicked, this, &AutoSaveWidget::onRebuildClicked);
}

void AutoSaveWidget::onToggleChanged(int state) {
  if (m_isRebuilding) return;
  qDebug() << state;
  m_isEnabled = (state == Qt::Checked);
  m_pathCombo->setEnabled(m_isEnabled);
  m_browseButton->setEnabled(m_isEnabled);
  m_openDirButton->setEnabled(m_isEnabled);
  m_cleanButton->setEnabled(m_isEnabled);
  m_rebuildButton->setEnabled(m_isEnabled);
  m_maxSizeSpinBox->setEnabled(m_isEnabled);
  if (m_pathLabel) m_pathLabel->setEnabled(m_isEnabled);
  if (m_maxSizeLabel) m_maxSizeLabel->setEnabled(m_isEnabled);
  saveSettings();
}

void AutoSaveWidget::onMaxSizeChanged(int value) {
  qDebug() << value;
  m_maxSizeMB = value;
  saveSettings();
}

void AutoSaveWidget::onBrowseClicked() {
  qDebug() << "clicked";
  QString dir = QFileDialog::getExistingDirectory(
      this, "Select Target Directory",
      m_targetDir.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) : m_targetDir);
  if (!dir.isEmpty()) {
    qDebug() << "New target directory selected" << dir;
    m_targetDir = dir;
    updateRecentPaths(m_targetDir);
    saveSettings();
    loadChecksums();
  }
}

void AutoSaveWidget::onOpenDirClicked() {
  if (m_targetDir.isEmpty()) return;
  QDesktopServices::openUrl(QUrl::fromLocalFile(m_targetDir));
}

void AutoSaveWidget::onPathSelected(int index) {
  QString path = m_pathCombo->itemText(index);

  if (path == kClearRecentPaths) {
    QString current = m_targetDir;
    m_recentPaths.clear();
    if (!current.isEmpty()) {
      m_recentPaths.append(current);
    }
    auto settings = m_manager->settingsManager();
    settings->setRecentAutoSavePaths(m_recentPaths);
    populatePathCombo();
    return;
  }

  if (path == m_targetDir) return;

  m_targetDir = path;
  // Move to top
  updateRecentPaths(m_targetDir);
  saveSettings();
  loadChecksums();
}

void AutoSaveWidget::updateRecentPaths(const QString& path) {
  if (path.isEmpty()) return;

  m_recentPaths.removeAll(path);
  m_recentPaths.prepend(path);
  while (m_recentPaths.size() > 10) {
    m_recentPaths.removeLast();
  }

  // Update Settings
  auto settings = m_manager->settingsManager();
  settings->setRecentAutoSavePaths(m_recentPaths);

  populatePathCombo();
}

void AutoSaveWidget::populatePathCombo() {
  bool blocked = m_pathCombo->blockSignals(true);
  m_pathCombo->clear();
  if (!m_recentPaths.isEmpty()) {
    m_pathCombo->addItems(m_recentPaths);
    if (m_recentPaths.size() > 1) {
      m_pathCombo->insertSeparator(m_pathCombo->count());
      m_pathCombo->addItem(kClearRecentPaths);
    }
  }

  int index = m_pathCombo->findText(m_targetDir);
  if (index != -1) {
    m_pathCombo->setCurrentIndex(index);
  }
  m_pathCombo->blockSignals(blocked);
}

void AutoSaveWidget::onCleanClicked() {
  if (m_targetDir.isEmpty()) {
    m_manager->logAction("Cannot clean checksums: Target directory is not set.", EventCategory::AutoSaveImage,
                         EventLevel::Warning);
    return;
  }

  QFile file(QDir(m_targetDir).filePath(kChecksumFileName));
  if (!file.exists()) {
    m_manager->logAction("Checksums file not found.", EventCategory::AutoSaveImage, EventLevel::Info);
    return;
  }

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_manager->logAction("Failed to open checksums file for reading.", EventCategory::AutoSaveImage, EventLevel::Error);
    return;
  }

  QStringList validLines;
  QSet<QByteArray> validChecksums;
  int removedCount = 0;

  QTextStream in(&file);
  while (!in.atEnd()) {
    QString line = in.readLine();
    if (line.trimmed().isEmpty()) continue;

    int splitIndex = line.indexOf('|');
    if (splitIndex != -1) {
      QString filename = line.mid(splitIndex + 1).trimmed();
      if (QFile::exists(QDir(m_targetDir).filePath(filename))) {
        validLines.append(line);
        QString hashHex = line.left(splitIndex);
        validChecksums.insert(QByteArray::fromHex(hashHex.toUtf8()));
      } else {
        removedCount++;
      }
    }
  }
  file.close();

  if (removedCount > 0) {
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
      QTextStream out(&file);
      for (const QString& line : validLines) {
        out << line << "\n";
      }
      file.close();

      m_seenChecksums = validChecksums;
      m_manager->logAction(QString("Cleaned checksums. Removed %1 non-existent entries.").arg(removedCount),
                           EventCategory::AutoSaveImage, EventLevel::Info);
    } else {
      m_manager->logAction("Failed to write cleaned checksums file.", EventCategory::AutoSaveImage, EventLevel::Error);
    }
  } else {
    m_manager->logAction("No non-existent entries found in checksums.", EventCategory::AutoSaveImage, EventLevel::Info);
  }
}

void AutoSaveWidget::onClipboardChanged() {
  if (!m_isEnabled || m_isRebuilding) return;
  qDebug() << "processing";

  bool savedFromText = processTextContent();
  if (savedFromText) {
    return;
  }
  bool savedFromImage = processImageContent();
  if (savedFromImage) {
    return;
  }
  qDebug() << "No valid content found in clipboard";
}

bool AutoSaveWidget::processTextContent() {
  if (!m_manager->hasText()) {
    return false;
  }

  // Check if text is a file path to an image
  QString text = m_manager->latestText();
  qDebug() << "Checking text content" << text;
  QUrl url(text);

  bool handled = handleRemoteUrl(url, m_manager->latestImage());
  if (handled) {
    return true;
  }

  QString path = url.isLocalFile() ? url.toLocalFile() : text;
  return handleLocalPath(path);
}

bool AutoSaveWidget::processImageContent() {
  if (!m_manager->hasImage()) {
    return false;
  }

  QImage image = m_manager->latestImage();
  qDebug() << "Checking image content" << image;
  return saveImage(image);
}

bool AutoSaveWidget::handleRemoteUrl(const QUrl& url, const QImage& fallbackImage) {
  if (!(url.isValid() && (url.scheme() == "http" || url.scheme() == "https"))) {
    return false;
  }

  qDebug() << "Detected HTTP URL" << url.toString();
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (compatible; ClipboardToolbox/1.0)");
  QNetworkReply* reply = m_networkManager->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply, url, fallbackImage]() {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      qDebug() << "Network error:" << reply->errorString();
      m_manager->logAction("Network error downloading image: " + reply->errorString(), EventCategory::AutoSaveImage,
                           EventLevel::Error);
    } else {
      QByteArray data = reply->readAll();
      qDebug() << "Downloaded data size:" << data.size() << "byte(s) from" << url.toString();

      if (!QImage::fromData(data).isNull()) {
        // Save original data to temp file to avoid re-encoding
        QTemporaryFile tempFile;
        if (tempFile.open()) {
          tempFile.write(data);
          tempFile.close();
          saveImage(tempFile, url.fileName(), url.toString());
          return;
        } else {
          qDebug() << "Failed to create temp file for URL image";
        }
      } else {
        qDebug() << "Downloaded data is not a valid image";
      }
    }

    qDebug() << "Using fallback image from clipboard";
    saveImage(fallbackImage);
  });
  return true;  // Assume handled asynchronously
}

bool AutoSaveWidget::handleLocalPath(const QString& path) {
  QFileInfo fi(path);
  if (fi.exists() && fi.isFile()) {
    qDebug() << "Text identifies a file" << path;
    QImageReader reader(path);
    if (reader.canRead()) {
      qint64 size = fi.size();
      if (size > m_maxSizeMB * 1024 * 1024) {
        qDebug() << "File size exceeds limit" << size;
        m_manager->logAction(QString("File size (%1 MB) exceeds limit (%2 MB): %3")
                                 .arg(size / 1024.0 / 1024.0, 0, 'f', 2)
                                 .arg(m_maxSizeMB)
                                 .arg(path),
                             EventCategory::AutoSaveImage, EventLevel::Warning);
        return true;  // Found valid image file but skipped due to size
      } else {
        QImage img = reader.read();
        if (!img.isNull()) {
          qDebug() << "Image loaded from file" << path;
          QFile file(path);
          return saveImage(file, fi.fileName(), path);
        } else {
          qDebug() << "Failed to load image from file" << path;
        }
      }
    } else {
      qDebug() << "QImageReader cannot read file" << path;
    }
  }
  return false;
}

bool AutoSaveWidget::saveImage(QFile& file, const QString& originalName, const QString& source) {
  qDebug() << "Processing save for:" << source << originalName;

  if (m_targetDir.isEmpty() || !QDir(m_targetDir).exists()) {
    qDebug() << "Target directory invalid" << m_targetDir;
    m_manager->logAction("Target directory invalid or does not exist: " + m_targetDir, EventCategory::AutoSaveImage,
                         EventLevel::Error);
    return false;
  }

  qint64 imageSize = file.size();

  // Check size limit
  if (imageSize > m_maxSizeMB * 1024 * 1024) {
    qDebug() << "Image file size exceeds limit";
    m_manager->logAction(QString("Image file size (%1 MB) exceeds limit (%2 MB).")
                             .arg(imageSize / 1024.0 / 1024.0, 0, 'f', 2)
                             .arg(m_maxSizeMB),
                         EventCategory::AutoSaveImage, EventLevel::Warning);
    return false;
  }

  // Check checksum
  if (!file.open(QIODevice::ReadOnly)) {
    qDebug() << "Failed to open source file for checksum:" << file.fileName();
    m_manager->logAction("Failed to open source file for checksum: " + file.fileName(), EventCategory::AutoSaveImage,
                         EventLevel::Error);
    return false;
  }
  QByteArray checksum = calculateChecksum(&file);
  file.close();

  if (checksum.isEmpty()) {
    qDebug() << "Failed to calculate checksum for source file:" << file.fileName();
    m_manager->logAction("Failed to calculate checksum for source file: " + file.fileName(),
                         EventCategory::AutoSaveImage, EventLevel::Error);
    return false;
  }

  if (m_seenChecksums.contains(checksum)) {
    qDebug() << "Duplicate image detected (checksum match). Skipping." << source;
    m_manager->logAction(QString("Duplicate image detected for %1. Skipping save.").arg(source),
                         EventCategory::AutoSaveImage, EventLevel::Warning);
    return false;
  }

  // Generate Destination Filename
  QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
  QString namePart = originalName.isEmpty() ? "clipboard.jpg" : originalName;
  QString filename = timestamp + "_" + namePart;
  QString fullPath = QDir(m_targetDir).filePath(filename);

  // Copy file
  if (file.copy(fullPath)) {
    qDebug() << "Image copied successfully to" << fullPath;
    m_seenChecksums.insert(checksum);
    appendChecksum(filename, checksum);
    m_manager->logAction(QString("%1 -> %2").arg(source).arg(fullPath), EventCategory::AutoSaveImage, EventLevel::Info);
    return true;
  } else {
    qDebug() << "Failed to copy image to" << fullPath;
    m_manager->logAction(QString("Failed to copy image to: %1").arg(fullPath), EventCategory::AutoSaveImage,
                         EventLevel::Error);
    return false;
  }
}

bool AutoSaveWidget::saveImage(const QImage& image) {
  if (image.isNull()) {
    qDebug() << "Failed to load image from clipboard";
    m_manager->logAction("Failed to load image from clipboard", EventCategory::AutoSaveImage, EventLevel::Error);
    return false;
  }

  // Save to temp file first to avoid re-encoding
  QTemporaryFile tempFile;
  if (tempFile.open()) {
    image.save(&tempFile, "JPG");
    tempFile.close();
    return saveImage(tempFile, "clipboard.jpg", "<Clipboard Image>");
  } else {
    qDebug() << "Failed to create temp file for clipboard image";
    m_manager->logAction("Failed to create temp file for clipboard image", EventCategory::AutoSaveImage,
                         EventLevel::Error);
    return false;
  }
}

void AutoSaveWidget::loadSettings() {
  auto settings = m_manager->settingsManager();
  m_isEnabled = settings->autoSaveEnabled();
  m_targetDir = settings->autoSavePath();
  m_recentPaths = settings->recentAutoSavePaths();
  m_maxSizeMB = settings->autoSaveMaxSizeMB();

  qDebug() << "Settings loaded. Enabled:" << m_isEnabled << "Path:" << m_targetDir << "MaxMB:" << m_maxSizeMB;

  if (m_isEnabled) {
    m_enableCheckBox->setChecked(true);
  }

  // Populate Combo
  if (!m_targetDir.isEmpty() && !m_recentPaths.contains(m_targetDir)) {
    updateRecentPaths(m_targetDir);
  } else {
    populatePathCombo();
  }

  m_maxSizeSpinBox->setValue(m_maxSizeMB);

  // Update state
  m_pathCombo->setEnabled(m_isEnabled);
  m_openDirButton->setEnabled(m_isEnabled);
}

void AutoSaveWidget::saveSettings() {
  auto settings = m_manager->settingsManager();
  settings->setAutoSaveEnabled(m_isEnabled);
  settings->setAutoSavePath(m_targetDir);
  settings->setAutoSaveMaxSizeMB(m_maxSizeMB);
}

QByteArray AutoSaveWidget::calculateChecksum(QIODevice* device) {
  if (!device || !device->isOpen() || !device->isReadable()) {
    qDebug() << "calculateChecksum: Invalid or closed device";
    return QByteArray();
  }

  QCryptographicHash hash(QCryptographicHash::Md5);
  if (hash.addData(device)) {
    return hash.result();
  }
  return QByteArray();
}

void AutoSaveWidget::loadChecksums() {
  m_seenChecksums.clear();
  if (m_targetDir.isEmpty()) return;

  QFile file(QDir(m_targetDir).filePath(kChecksumFileName));
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    while (!in.atEnd()) {
      QString line = in.readLine();
      int splitIndex = line.indexOf(": ");
      if (splitIndex != -1) {
        QString hashHex = line.mid(splitIndex + 2).trimmed();
        m_seenChecksums.insert(QByteArray::fromHex(hashHex.toUtf8()));
      }
    }
  } else {
    qDebug() << "loadChecksums: Failed to open checksums.txt" << file.errorString();
    m_manager->logAction("Failed to load checksums: " + file.errorString(), EventCategory::AutoSaveImage,
                         EventLevel::Warning);
  }
  qDebug() << "Loaded" << m_seenChecksums.size() << "checksums";
}

void AutoSaveWidget::appendChecksum(const QString& filename, const QByteArray& checksum) {
  if (m_targetDir.isEmpty()) return;

  QFile file(QDir(m_targetDir).filePath(kChecksumFileName));
  if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    QTextStream out(&file);
    out << filename << ": " << checksum.toHex() << "\n";
  } else {
    qDebug() << "appendChecksum: Failed to open checksums.txt for appending" << file.errorString();
    m_manager->logAction("Failed to append checksum: " + file.errorString(), EventCategory::AutoSaveImage,
                         EventLevel::Error);
  }
}

void AutoSaveWidget::onRebuildClicked() {
  if (m_targetDir.isEmpty() || !QDir(m_targetDir).exists()) {
    m_manager->logAction("Target directory invalid, cannot rebuild.", EventCategory::AutoSaveImage, EventLevel::Error);
    return;
  }

  m_isRebuilding = true;
  m_enableCheckBox->setEnabled(false);
  m_pathCombo->setEnabled(false);
  m_browseButton->setEnabled(false);
  m_openDirButton->setEnabled(false);
  m_cleanButton->setEnabled(false);
  m_rebuildButton->setEnabled(false);
  m_maxSizeSpinBox->setEnabled(false);
  m_progressBar->setVisible(true);
  m_progressBar->setValue(0);

  QDir dir(m_targetDir);
  QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
  m_progressBar->setRange(0, files.size());

  m_seenChecksums.clear();
  QFile file(dir.filePath(kChecksumFileName));
  if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    QTextStream out(&file);

    int processed = 0;
    for (const QString& filename : files) {
      if (filename == kChecksumFileName) continue;

      QImageReader reader(dir.filePath(filename));
      if (reader.canRead()) {
        QFile file(dir.filePath(filename));
        if (file.open(QIODevice::ReadOnly)) {
          QByteArray checksum = calculateChecksum(&file);
          file.close();
          if (!checksum.isEmpty()) {
            m_seenChecksums.insert(checksum);
            out << filename << ": " << checksum.toHex() << "\n";
          } else {
            m_manager->logAction("Failed to calculate checksum for file: " + filename, EventCategory::AutoSaveImage,
                                 EventLevel::Warning);
          }
        }
      }
      processed++;
      m_progressBar->setValue(processed);
      QCoreApplication::processEvents();
    }
    file.close();
    m_manager->logAction(QString("Rebuilt checksums for %1 files.").arg(processed), EventCategory::AutoSaveImage,
                         EventLevel::Info);
  } else {
    m_manager->logAction("Failed to open checksums file for writing.", EventCategory::AutoSaveImage, EventLevel::Error);
  }

  m_isRebuilding = false;
  m_progressBar->setVisible(false);
  m_enableCheckBox->setEnabled(true);
  // Restore enabled state based on checkbox
  onToggleChanged(m_enableCheckBox->isChecked() ? Qt::Checked : Qt::Unchecked);
}
