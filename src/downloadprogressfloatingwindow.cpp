#include "downloadprogressfloatingwindow.h"

#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QPainter>
#include <QScreen>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>

#include "downloadprogressmodel.h"

// Filter Model to hide finished downloads
class ActiveDownloadsFilter : public QSortFilterProxyModel {
 public:
  ActiveDownloadsFilter(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

 protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override {
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    bool isFinished = sourceModel()->data(index, DownloadProgressModel::IsFinishedRole).toBool();
    bool isError = sourceModel()->data(index, DownloadProgressModel::IsErrorRole).toBool();
    // Show if not finished OR if it finished with error (user might want to see errors)
    // User asked: "floating window doesn't display the successful finished progress"
    // So we show active AND errors.
    return !isFinished || isError;
  }
};

// Delegate for floating window (similar to main window but maybe compact)
class FloatingDownloadDelegate : public QStyledItemDelegate {
 public:
  FloatingDownloadDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
    if (!index.isValid()) return;

    painter->save();

    // Background
    if (option.state & QStyle::State_Selected) {
      painter->fillRect(option.rect, option.palette.highlight());
    } else {
      painter->fillRect(option.rect, option.palette.base());
    }

    QRect r = option.rect;
    r.adjust(5, 5, -5, -5);

    QString url = index.data(DownloadProgressModel::UrlRole).toUrl().fileName();
    if (url.isEmpty()) url = index.data(DownloadProgressModel::UrlRole).toUrl().toString();

    int progress = index.data(DownloadProgressModel::ProgressRole).toInt();
    QString status = index.data(DownloadProgressModel::StatusRole).toString();
    bool isError = index.data(DownloadProgressModel::IsErrorRole).toBool();

    // Layout: Single line
    // [FileName (150px)] [ProgressBar (flex)] [Status (50px)]

    int spacing = 5;
    int nameWidth = 150;
    int statusWidth = 50;

    QRect rect = r;

    // Filename (Left)
    QRect nameRect(rect.left(), rect.top(), nameWidth, rect.height());

    // Status (Right)
    QRect statusRect(rect.right() - statusWidth, rect.top(), statusWidth, rect.height());

    // Progress Bar (Remaining Middle)
    int barLeft = nameRect.right() + spacing;
    int barWidth = statusRect.left() - spacing - barLeft;
    // Ensure bar width is not negative
    barWidth = qMax(0, barWidth);

    QRect barRect(barLeft, rect.top() + (rect.height() - 12) / 2, barWidth, 12);

    // Draw Text (Filename)
    painter->setPen(option.palette.text().color());
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(url, Qt::ElideMiddle, nameRect.width()));

    // Draw Progress Bar
    QStyleOptionProgressBar progressBarOption;
    progressBarOption.rect = barRect;
    progressBarOption.minimum = 0;
    progressBarOption.maximum = 100;
    progressBarOption.progress = progress;
    progressBarOption.textVisible = false;

    QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);

    // Draw Status/Percentage on the right
    QString statusText = isError ? "Error" : QString("%1%").arg(progress);
    if (isError) painter->setPen(Qt::red);
    painter->drawText(statusRect, Qt::AlignRight | Qt::AlignVCenter, statusText);

    painter->restore();
  }

  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
    return QSize(option.rect.width(), 30);
  }
};

DownloadProgressFloatingWindow::DownloadProgressFloatingWindow(QWidget *parent) : QDialog(nullptr) {
  setWindowTitle("Downloads");
  setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::Tool);  // Tool window has smaller title bar
  setAttribute(Qt::WA_ShowWithoutActivating);                        // Don't steal focus
  resize(350, 200);

  // Position at bottom right of primary screen
  QRect screenGeometry = QApplication::primaryScreen()->availableGeometry();
  move(screenGeometry.right() - width(), screenGeometry.top());

  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  m_listView = new QListView(this);
  m_listView->setItemDelegate(new FloatingDownloadDelegate(this));
  layout->addWidget(m_listView);

  m_emptyLabel = new QLabel("No active downloads", this);
  m_emptyLabel->setAlignment(Qt::AlignCenter);
  layout->addWidget(m_emptyLabel);
  m_emptyLabel->hide();
}

DownloadProgressFloatingWindow::~DownloadProgressFloatingWindow() {}

void DownloadProgressFloatingWindow::setModel(QAbstractItemModel *model) {
  ActiveDownloadsFilter *proxy = new ActiveDownloadsFilter(this);
  proxy->setSourceModel(model);
  m_listView->setModel(proxy);

  auto updateView = [this, proxy]() {
    if (proxy->rowCount() == 0) {
      m_listView->hide();
      m_emptyLabel->show();
    } else {
      m_listView->show();
      m_emptyLabel->hide();
    }
  };

  // Initial update
  updateView();

  // Auto-hide if empty?
  connect(proxy, &QAbstractItemModel::rowsInserted, this, [this, proxy, updateView]() {
    updateView();
    if (proxy->rowCount() > 0 && !isVisible()) show();
  });
  connect(proxy, &QAbstractItemModel::rowsRemoved, this, [this, proxy, updateView]() {
    updateView();
    QTimer::singleShot(2000, this, [this, proxy]() {
      if (proxy->rowCount() == 0 && isVisible()) hide();
    });
  });
}
