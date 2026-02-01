#include "historywidget.h"

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include "clipboardmanager.h"
#include "event.h"
#include "historymanager.h"

HistoryWidget::HistoryWidget(ClipboardManager *manager, QWidget *parent)
    : QWidget(parent), m_manager(manager) {
  setupUi();
  reloadHistory();

  connect(m_manager->historyManager(), &QAbstractListModel::rowsInserted, this,
          [this](const QModelIndex &, int first, int last) {
            auto *model = m_manager->historyManager();
            for (int i = first; i <= last; ++i) {
              const auto &item = model->eventAt(i);
              if (isEventVisible(item)) {
                addRow(m_historyTable->rowCount(), item);
              }
            }
            m_historyTable->scrollToBottom();
          });

  connect(m_manager->historyManager(), &QAbstractListModel::modelReset, this,
          &HistoryWidget::reloadHistory);
}

void HistoryWidget::setupUi() {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto *toolbarLayout = new QHBoxLayout();
  toolbarLayout->setContentsMargins(4, 4, 4, 4);
  toolbarLayout->setSpacing(3);

  auto *titleLabel = new QLabel("History Log", this);
  QFont font = titleLabel->font();
  font.setBold(true);
  titleLabel->setFont(font);
  toolbarLayout->addWidget(titleLabel);

  toolbarLayout->addStretch();

  // Level Filter
  m_levelFilterBtn = new QToolButton(this);
  m_levelFilterBtn->setPopupMode(QToolButton::InstantPopup);
  m_levelFilterBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  auto *levelMenu = new QMenu(m_levelFilterBtn);

  auto addLevelAction = [&](const QString &text, int level) {
    auto *action = levelMenu->addAction(text);
    action->setCheckable(true);
    action->setChecked(true);
    action->setData(level);
    m_levelActions.append(action);
    connect(action, &QAction::triggered, this, &HistoryWidget::reloadHistory);
    connect(action, &QAction::triggered, this,
            &HistoryWidget::updateFilterButtonText);
  };

  addLevelAction("Info", (int)EventLevel::Info);
  addLevelAction("Warning", (int)EventLevel::Warning);
  addLevelAction("Error", (int)EventLevel::Error);

  m_levelFilterBtn->setMenu(levelMenu);
  toolbarLayout->addWidget(new QLabel("Level:"));
  toolbarLayout->addWidget(m_levelFilterBtn);

  // Category Filter
  m_categoryFilterBtn = new QToolButton(this);
  m_categoryFilterBtn->setPopupMode(QToolButton::InstantPopup);
  m_categoryFilterBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  auto *categoryMenu = new QMenu(m_categoryFilterBtn);

  auto addCategoryAction = [&](const QString &text, int category) {
    auto *action = categoryMenu->addAction(text);
    action->setCheckable(true);
    action->setChecked(true);
    action->setData(category);
    m_categoryActions.append(action);
    connect(action, &QAction::triggered, this, &HistoryWidget::reloadHistory);
    connect(action, &QAction::triggered, this,
            &HistoryWidget::updateFilterButtonText);
  };

  addCategoryAction("Copy", (int)EventCategory::Copy);
  addCategoryAction("AutoSaveImage", (int)EventCategory::AutoSaveImage);

  m_categoryFilterBtn->setMenu(categoryMenu);
  toolbarLayout->addWidget(new QLabel("Category:"));
  toolbarLayout->addWidget(m_categoryFilterBtn);

  updateFilterButtonText();

  auto *clearBtn = new QPushButton("Clear History");
  connect(clearBtn, &QPushButton::clicked, m_manager->historyManager(),
          &HistoryManager::clear);
  toolbarLayout->addWidget(clearBtn);

  layout->addLayout(toolbarLayout);

  m_historyTable = new QTableWidget();
  m_historyTable->setColumnCount(4);
  m_historyTable->setHorizontalHeaderLabels(
      {"Time", "Level", "Category", "Content"});
  m_historyTable->horizontalHeader()->setSectionResizeMode(
      0, QHeaderView::ResizeToContents);
  m_historyTable->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  m_historyTable->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  m_historyTable->horizontalHeader()->setSectionResizeMode(
      3, QHeaderView::Stretch);
  m_historyTable->setWordWrap(false);
  m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_historyTable->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_historyTable, &QWidget::customContextMenuRequested, this,
          &HistoryWidget::showContextMenu);

  layout->addWidget(m_historyTable);
}

void HistoryWidget::updateFilterButtonText() {
  auto updateText = [](QToolButton *btn, const QList<QAction *> &actions) {
    QStringList selected;
    for (auto *action : actions) {
      if (action->isChecked()) {
        selected << action->text();
      }
    }
    if (selected.size() == actions.size()) {
      btn->setText("All");
    } else if (selected.isEmpty()) {
      btn->setText("None");
    } else if (selected.size() <= 2) {
      btn->setText(selected.join(", "));
    } else {
      btn->setText(QString("%1 selected").arg(selected.size()));
    }
  };

  updateText(m_levelFilterBtn, m_levelActions);
  updateText(m_categoryFilterBtn, m_categoryActions);
}

void HistoryWidget::reloadHistory() {
  m_historyTable->setRowCount(0);
  auto *model = m_manager->historyManager();
  for (int i = 0; i < model->rowCount(); ++i) {
    const auto &item = model->eventAt(i);
    if (isEventVisible(item)) {
      addRow(m_historyTable->rowCount(), item);
    }
  }
  m_historyTable->scrollToBottom();
}

bool HistoryWidget::isEventVisible(const EventItem &item) const {
  bool levelMatch = false;
  for (auto *action : m_levelActions) {
    if (action->isChecked() && action->data().toInt() == (int)item.level) {
      levelMatch = true;
      break;
    }
  }

  bool categoryMatch = false;
  for (auto *action : m_categoryActions) {
    if (action->isChecked() && action->data().toInt() == (int)item.category) {
      categoryMatch = true;
      break;
    }
  }

  return levelMatch && categoryMatch;
}

void HistoryWidget::addRow(int row, const EventItem &item) {
  m_historyTable->insertRow(row);

  auto *timeItem =
      new QTableWidgetItem(item.time.toString("yyyy-MM-dd HH:mm:ss"));
  timeItem->setTextAlignment(Qt::AlignCenter);
  m_historyTable->setItem(row, 0, timeItem);

  QIcon icon;
  switch (item.level) {
    case EventLevel::Info:
      icon = style()->standardIcon(QStyle::SP_MessageBoxInformation);
      break;
    case EventLevel::Warning:
      icon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
      break;
    case EventLevel::Error:
      icon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
      break;
    default:
      break;
  }

  auto *levelLabel = new QLabel();
  levelLabel->setPixmap(icon.pixmap(16, 16));
  levelLabel->setAlignment(Qt::AlignCenter);
  levelLabel->setToolTip(HistoryManager::levelToString(item.level));
  m_historyTable->setCellWidget(row, 1, levelLabel);

  auto *categoryItem =
      new QTableWidgetItem(HistoryManager::categoryToString(item.category));
  categoryItem->setTextAlignment(Qt::AlignCenter);
  m_historyTable->setItem(row, 2, categoryItem);

  auto *contentItem = new QTableWidgetItem(item.content);
  contentItem->setToolTip(item.content);
  m_historyTable->setItem(row, 3, contentItem);
}

void HistoryWidget::showContextMenu(const QPoint &pos) {
  auto *menu = new QMenu(this);
  auto *copyAction = menu->addAction("Copy");
  connect(copyAction, &QAction::triggered, this, [this, pos]() {
    int row = m_historyTable->rowAt(pos.y());
    if (row >= 0) {
      QStringList parts;
      for (int i = 0; i < m_historyTable->columnCount(); ++i) {
        if (auto *widget = m_historyTable->cellWidget(row, i)) {
          // Handle cell widget (e.g., the level icon label)
          if (auto *label = qobject_cast<QLabel *>(widget)) {
            parts << label->toolTip();
          }
        } else if (auto *item = m_historyTable->item(row, i)) {
          parts << item->text();
        }
      }
      qDebug() << parts;
      QApplication::clipboard()->setText(parts.join("\t"));
    }
  });
  menu->exec(m_historyTable->viewport()->mapToGlobal(pos));
  delete menu;
}
