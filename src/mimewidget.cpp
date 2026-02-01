#include "mimewidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMimeData>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include "clipboardmanager.h"

MimeWidget::MimeWidget(ClipboardManager *manager, QWidget *parent)
    : QWidget(parent), m_manager(manager) {
  setupUi();
  updateContent();

  connect(m_manager, &ClipboardManager::clipboardChanged, this,
          &MimeWidget::updateContent);
}

void MimeWidget::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  m_mimeTable = new QTableWidget(0, 2);
  m_mimeTable->setHorizontalHeaderLabels({"MIME Type", "Content"});
  m_mimeTable->horizontalHeader()->setStretchLastSection(true);
  m_mimeTable->verticalHeader()->setVisible(false);
  m_mimeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_mimeTable->setSelectionMode(QAbstractItemView::SingleSelection);

  mainLayout->addWidget(m_mimeTable);

  connect(m_mimeTable, &QTableWidget::cellDoubleClicked, this,
          [this](int row, int column) {
            if (column != 1) return;
            auto *item = m_mimeTable->item(row, column);
            if (!item) return;

            // Check if elided
            // 1. Data size check (if > 4096)
            QString format = m_mimeTable->item(row, 0)->text();
            const QMimeData *mime = m_manager->mimeData();
            if (mime && mime->hasFormat(format)) {
              QByteArray data = mime->data(format);
              if (data.size() > 4096) {
                showFullMimeContent(format);
                return;
              }
            }

            // 2. Visual check
            int colWidth = m_mimeTable->columnWidth(column);
            QFontMetrics fm(m_mimeTable->font());
            int textWidth = fm.horizontalAdvance(item->text());
            if (textWidth > colWidth - 10) {  // Simple margin
              showFullMimeContent(format);
            }
          });
}

void MimeWidget::updateContent() {
  const QMimeData *mime = m_manager->mimeData();
  if (mime) {
    QStringList formats = mime->formats();
    m_mimeTable->setRowCount(0);

    for (const QString &format : formats) {
      int row = m_mimeTable->rowCount();
      m_mimeTable->insertRow(row);
      m_mimeTable->setItem(row, 0, new QTableWidgetItem(format));

      QByteArray data = mime->data(format);
      QByteArray previewData = data.left(4096);

      QString displayStr;
      bool isBinary = false;

      // Try UTF-8
      QString utf8Str = QString::fromUtf8(previewData);
      // Try UTF-16
      QString utf16Str = QString::fromUtf16(
          reinterpret_cast<const ushort *>(previewData.constData()),
          previewData.size() / 2);

      bool utf8Valid = !utf8Str.contains(QChar::ReplacementCharacter) &&
                       !utf8Str.contains('\0');
      bool utf16Valid = !utf16Str.contains(QChar::ReplacementCharacter) &&
                        !utf16Str.contains('\0');

      if (utf8Valid) {
        displayStr = utf8Str;
      } else if (utf16Valid) {
        displayStr = utf16Str;
      } else {
        if (!previewData.contains('\0')) {
          displayStr = QString::fromUtf8(previewData);
        } else {
          isBinary = true;
          displayStr = previewData.toHex(' ');
        }
      }

      if (!isBinary) {
        displayStr = displayStr.toHtmlEscaped();
        displayStr.replace('\n', ' ');
        displayStr.replace('\r', ' ');
      }

      auto *item = new QTableWidgetItem(displayStr);
      item->setToolTip(displayStr);
      m_mimeTable->setItem(row, 1, item);
    }
  } else {
    m_mimeTable->setRowCount(0);
  }
}

void MimeWidget::showFullMimeContent(const QString &format) {
  const QMimeData *mime = m_manager->mimeData();
  if (!mime || !mime->hasFormat(format)) return;

  QByteArray data = mime->data(format);

  QDialog dialog(this);
  dialog.setWindowTitle("Full MIME Content - " + format);
  dialog.resize(600, 400);

  auto *layout = new QVBoxLayout(&dialog);
  auto *textEdit = new QTextEdit(&dialog);
  textEdit->setReadOnly(true);

  // Decide how to show full content
  QString displayStr;
  bool isBinary = false;

  // Try UTF-8
  QString utf8Str = QString::fromUtf8(data);
  // Try UTF-16
  QString utf16Str = QString::fromUtf16(
      reinterpret_cast<const ushort *>(data.constData()), data.size() / 2);

  bool utf8Valid =
      !utf8Str.contains(QChar::ReplacementCharacter) && !utf8Str.contains('\0');
  bool utf16Valid = !utf16Str.contains(QChar::ReplacementCharacter) &&
                    !utf16Str.contains('\0');

  if (utf8Valid) {
    displayStr = utf8Str;
  } else if (utf16Valid) {
    displayStr = utf16Str;
  } else {
    // Fallback
    if (!data.contains('\0')) {
      displayStr = QString::fromUtf8(data);
    } else {
      isBinary = true;
      displayStr = data.toHex(' ');
    }
  }

  textEdit->setPlainText(displayStr);

  layout->addWidget(textEdit);

  auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::accept);
  layout->addWidget(buttonBox);

  dialog.exec();
}
