#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>

#include "event.h"

class HistoryManager : public QAbstractListModel {
  Q_OBJECT

 public:
  explicit HistoryManager(QObject *parent = nullptr);
  ~HistoryManager() override;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;

  void addEvent(const EventItem &item);
  void addEntry(const QString &content, EventCategory category, EventLevel level);
  void clear();
  const EventItem &eventAt(int index) const;

  // Helper to convert enums to string for display/storage
  static QString categoryToString(EventCategory category);
  static QString levelToString(EventLevel level);
  static EventCategory stringToCategory(const QString &str);
  static EventLevel stringToLevel(const QString &str);

 private:
  void loadHistory();
  void saveHistory() const;

  QList<EventItem> m_entries;
  int m_maxEntries = 50;  // Default limit
};
