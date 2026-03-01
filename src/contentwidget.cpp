#include "contentwidget.h"

#include <QBuffer>
#include <QByteArray>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMetaEnum>
#include <QTableWidget>
#include <QTextEdit>
#include <QThreadPool>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "clipboardmanager.h"
#include "utils.h"

ContentWidget::ContentWidget(ClipboardManager *manager, QWidget *parent) : QWidget(parent), m_manager(manager) {
  setupUi();
  updateContent();

  connect(m_manager, &ClipboardManager::clipboardChanged, this, &ContentWidget::updateContent);
}

void ContentWidget::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(12, 12, 12, 12);
  mainLayout->setSpacing(12);

  // Image View Container (Horizontal Layout)
  auto *imageContainer = new QWidget();
  auto *imageLayout = new QHBoxLayout(imageContainer);
  imageLayout->setContentsMargins(0, 0, 0, 0);
  imageLayout->setSpacing(12);
  mainLayout->addWidget(imageContainer);

  m_imageLabel = new QLabel();
  m_imageLabel->setMinimumHeight(240);
  m_imageLabel->setAlignment(Qt::AlignCenter);
  m_imageLabel->setVisible(false);
  m_imageLabel->setContextMenuPolicy(Qt::CustomContextMenu);
  imageLayout->addWidget(m_imageLabel, 1);  // Stretch factor 1

  // Image Info Table
  m_imageInfoTable = new QTableWidget(0, 2);
  m_imageInfoTable->setHorizontalHeaderLabels({"Property", "Value"});
  m_imageInfoTable->horizontalHeader()->setStretchLastSection(true);
  m_imageInfoTable->verticalHeader()->setVisible(false);
  m_imageInfoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_imageInfoTable->setSelectionMode(QAbstractItemView::NoSelection);
  m_imageInfoTable->setVisible(false);
  m_imageInfoTable->setFixedWidth(250);         // Fixed width for info table
  imageLayout->addWidget(m_imageInfoTable, 0);  // Stretch factor 0

  m_textViewer = new QTextEdit();
  m_textViewer->setReadOnly(true);
  m_textViewer->setVisible(false);
  mainLayout->addWidget(m_textViewer);

  m_emptyClipboardLabel = new QLabel("Clipboard is empty or unsupported content");
  m_emptyClipboardLabel->setAlignment(Qt::AlignCenter);
  m_emptyClipboardLabel->setStyleSheet("color: gray;");
  mainLayout->addWidget(m_emptyClipboardLabel);

  connect(m_imageLabel, &QLabel::customContextMenuRequested, this, [this](const QPoint &pos) {
    QMenu menu(this);
    QAction *saveAction = menu.addAction("Save Image As...");
    saveAction->setEnabled(m_manager->hasImage());
    QAction *selected = menu.exec(m_imageLabel->mapToGlobal(pos));
    if (selected == saveAction) {
      QString fileName =
          QFileDialog::getSaveFileName(this, "Save image", QString(), "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
      if (!fileName.isEmpty()) {
        m_manager->saveImageToFile(fileName);
      }
    }
  });
}

ImageSizeResult ContentWidget::calculateImageSizes(QImage image) {
  ImageSizeResult result;

  QByteArray pngData;
  QBuffer pngBuffer(&pngData);
  pngBuffer.open(QIODevice::WriteOnly);
  image.save(&pngBuffer, "PNG");
  result.pngSize = pngData.size();

  QByteArray jpgData;
  QBuffer jpgBuffer(&jpgData);
  jpgBuffer.open(QIODevice::WriteOnly);
  image.save(&jpgBuffer, "JPG");
  result.jpgSize = jpgData.size();

  return result;
}

void ContentWidget::updateImageSizeInfo(const ImageSizeResult &result) {
  if (m_pngSizeRow >= 0 && m_pngSizeRow < m_imageInfoTable->rowCount()) {
    auto *item = m_imageInfoTable->item(m_pngSizeRow, 1);
    if (item) {
      item->setText(utils::formatSize(result.pngSize));
      item->setToolTip(QString::number(result.pngSize) + " bytes");
    }
  }

  if (m_jpgSizeRow >= 0 && m_jpgSizeRow < m_imageInfoTable->rowCount()) {
    auto *item = m_imageInfoTable->item(m_jpgSizeRow, 1);
    if (item) {
      item->setText(utils::formatSize(result.jpgSize));
      item->setToolTip(QString::number(result.jpgSize) + " bytes");
    }
  }
}

void ContentWidget::updateContent() {
  bool hasImage = m_manager->hasImage();
  bool hasText = m_manager->hasText();

  if (hasImage) {
    QImage image = m_manager->latestImage();
    if (!image.isNull()) {
      QSize targetSize = m_imageLabel->size();
      // Ensure target size is valid
      if (targetSize.width() <= 0 || targetSize.height() <= 0) {
        targetSize = QSize(640, 240);
      }
      QPixmap pixmap = QPixmap::fromImage(image).scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      m_imageLabel->setPixmap(pixmap);
      m_imageLabel->setVisible(true);

      // Populate Image Info Table
      m_imageInfoTable->setRowCount(0);
      m_imageInfoTable->setVisible(true);

      auto addRow = [this](const QString &key, const QString &value, const QString &tooltip = QString()) {
        int row = m_imageInfoTable->rowCount();
        m_imageInfoTable->insertRow(row);
        m_imageInfoTable->setItem(row, 0, new QTableWidgetItem(key));
        auto *valueItem = new QTableWidgetItem(value);
        if (!tooltip.isEmpty()) {
          valueItem->setToolTip(tooltip);
        }
        m_imageInfoTable->setItem(row, 1, valueItem);
        return row;
      };

      addRow("Width", QString::number(image.width()));
      addRow("Height", QString::number(image.height()));
      addRow("Depth", QString::number(image.depth()) + " bit");

      addRow("Raw Size", utils::formatSize(image.sizeInBytes()), QString::number(image.sizeInBytes()) + " bytes");

      // PNG Size
      // Note: The size here might differ from the original file size because:
      // 1. The clipboard image is likely converted to 32-bit ARGB (uncompressed
      // in memory).
      // 2. Saving as PNG uses Qt's default compression (zlib level -1, usually
      // 6).
      // 3. Original files might be 8-bit indexed or highly optimized.
      m_pngSizeRow = addRow("PNG Size", "Calculating...");

      // JPG Size
      m_jpgSizeRow = addRow("JPG Size", "Calculating...");

      QString format = "Bitmap";
      // Use QMetaEnum to convert QImage::Format enum to string
      QMetaEnum metaEnum = QMetaEnum::fromType<QImage::Format>();
      if (metaEnum.isValid()) {
        const char *key = metaEnum.valueToKey(image.format());
        if (key) {
          format = QString(key);
        }
      }
      addRow("Format", format);

      QThreadPool::globalInstance()->start([this, image]() {
        ImageSizeResult result = calculateImageSizes(image);
        QMetaObject::invokeMethod(this, [this, result]() { updateImageSizeInfo(result); }, Qt::QueuedConnection);
      });
    } else {
      m_imageLabel->setVisible(false);
      m_imageInfoTable->setVisible(false);
    }
  } else {
    m_imageLabel->setVisible(false);
    m_imageInfoTable->setVisible(false);
  }

  if (hasText) {
    m_textViewer->setText(m_manager->latestText());
    m_textViewer->setVisible(true);
  } else {
    m_textViewer->clear();
    m_textViewer->setVisible(false);
  }

  bool showEmpty = !hasImage && !hasText;
  m_emptyClipboardLabel->setVisible(showEmpty);
}

void ContentWidget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  // Re-scale image when window is resized
  if (m_manager->hasImage() && m_imageLabel->isVisible()) {
    QImage image = m_manager->latestImage();
    if (!image.isNull()) {
      QSize targetSize = m_imageLabel->size();
      if (targetSize.width() <= 0 || targetSize.height() <= 0) {
        return;
      }
      QPixmap pixmap = QPixmap::fromImage(image).scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
      m_imageLabel->setPixmap(pixmap);
    }
  }
}
