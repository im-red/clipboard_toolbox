#pragma once

#include <QWidget>

class ClipboardManager;
class QTableWidget;

class MimeWidget : public QWidget {
  Q_OBJECT

 public:
  explicit MimeWidget(ClipboardManager *manager, QWidget *parent = nullptr);

 private:
  void setupUi();
  void updateContent();
  void showFullMimeContent(const QString &format);

  ClipboardManager *m_manager = nullptr;
  QTableWidget *m_mimeTable = nullptr;
};
