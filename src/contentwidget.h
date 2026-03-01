#pragma once

#include <QImage>
#include <QWidget>

class ClipboardManager;
class QLabel;
class QTableWidget;
class QTextEdit;

struct ImageSizeResult {
  qint64 pngSize = 0;
  qint64 jpgSize = 0;
};

class ContentWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ContentWidget(ClipboardManager *manager, QWidget *parent = nullptr);

 protected:
  void resizeEvent(QResizeEvent *event) override;

 private:
  void setupUi();
  void updateContent();
  void updateImageSizeInfo(const ImageSizeResult &result);
  static ImageSizeResult calculateImageSizes(QImage image);

  ClipboardManager *m_manager = nullptr;
  QLabel *m_imageLabel = nullptr;
  QLabel *m_emptyClipboardLabel = nullptr;
  QTextEdit *m_textViewer = nullptr;
  QTableWidget *m_imageInfoTable = nullptr;
  int m_pngSizeRow = -1;
  int m_jpgSizeRow = -1;
};
