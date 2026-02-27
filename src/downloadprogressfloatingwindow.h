#pragma once

#include <QAbstractItemModel>
#include <QDialog>
#include <QListView>

class DownloadProgressFloatingWindow : public QDialog {
  Q_OBJECT

 public:
  explicit DownloadProgressFloatingWindow(QWidget *parent = nullptr);
  virtual ~DownloadProgressFloatingWindow();

  void setModel(QAbstractItemModel *model);

 private:
  QListView *m_listView;
};
