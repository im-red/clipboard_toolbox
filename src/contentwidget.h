#pragma once

#include <QWidget>

class ClipboardManager;
class QLabel;
class QTableWidget;
class QTextEdit;

class ContentWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ContentWidget(ClipboardManager *manager, QWidget *parent = nullptr);

 protected:
  void resizeEvent(QResizeEvent *event) override;

 private:
  void setupUi();
  void updateContent();

  ClipboardManager *m_manager = nullptr;
  QLabel *m_imageLabel = nullptr;
  QLabel *m_emptyClipboardLabel = nullptr;
  QTextEdit *m_textViewer = nullptr;
  QTableWidget *m_imageInfoTable = nullptr;
};
