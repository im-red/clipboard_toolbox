#pragma once

#include <QWidget>

#include "event.h"

class QTableWidget;
class QToolButton;
class QAction;
class ClipboardManager;

class HistoryWidget : public QWidget {
  Q_OBJECT

 public:
  explicit HistoryWidget(ClipboardManager *manager, QWidget *parent = nullptr);

 private:
  void setupUi();
  void reloadHistory();
  void addRow(int row, const EventItem &item);
  void showContextMenu(const QPoint &pos);
  bool isEventVisible(const EventItem &item) const;
  void updateFilterButtonText();

  ClipboardManager *m_manager = nullptr;
  QTableWidget *m_historyTable = nullptr;

  QToolButton *m_levelFilterBtn = nullptr;
  QList<QAction *> m_levelActions;

  QToolButton *m_categoryFilterBtn = nullptr;
  QList<QAction *> m_categoryActions;
};
