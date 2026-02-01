#include "historymanager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

HistoryManager::HistoryManager(QObject *parent) : QAbstractListModel(parent) {
  loadHistory();
}

HistoryManager::~HistoryManager() { saveHistory(); }

int HistoryManager::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_entries.size();
}

QVariant HistoryManager::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
    return {};
  }
  const EventItem &entry = m_entries.at(index.row());
  if (role != Qt::DisplayRole) {
    return {};
  }
  // Format: <TIMESTAMP>: [<LEVEL>] [<CATEGORY>] <CONTENT>
  return QString("%1: [%2] [%3] %4")
      .arg(entry.time.toString("yyyy-MM-dd HH:mm:ss"),
           levelToString(entry.level), categoryToString(entry.category),
           entry.content);
}

void HistoryManager::addEvent(const EventItem &item) {
  int count = m_entries.size();
  beginInsertRows(QModelIndex(), count, count);
  m_entries.append(item);
  endInsertRows();

  if (m_maxEntries > 0 && m_entries.size() > m_maxEntries) {
    beginRemoveRows(QModelIndex(), 0, 0);
    m_entries.removeFirst();
    endRemoveRows();
  }
  saveHistory();
}

void HistoryManager::addEntry(const QString &content, EventCategory category,
                              EventLevel level) {
  EventItem item;
  item.time = QDateTime::currentDateTime();
  item.category = category;
  item.level = level;
  item.content = content;
  addEvent(item);
}

void HistoryManager::clear() {
  beginResetModel();
  m_entries.clear();
  endResetModel();
  saveHistory();
}

const EventItem &HistoryManager::eventAt(int index) const {
  if (index < 0 || index >= m_entries.size()) {
    static EventItem empty;
    return empty;
  }
  return m_entries.at(index);
}

QString HistoryManager::categoryToString(EventCategory category) {
  switch (category) {
    case EventCategory::Copy:
      return "Copy";
    case EventCategory::AutoSaveImage:
      return "AutoSaveImage";
    case EventCategory::Invalid:
      return "Invalid";
    default:
      return "Unknown";
  }
}

QString HistoryManager::levelToString(EventLevel level) {
  switch (level) {
    case EventLevel::Info:
      return "Info";
    case EventLevel::Warning:
      return "Warning";
    case EventLevel::Error:
      return "Error";
    case EventLevel::Invalid:
      return "Invalid";
    default:
      return "Unknown";
  }
}

EventCategory HistoryManager::stringToCategory(const QString &str) {
  if (str == "AutoSaveImage") return EventCategory::AutoSaveImage;
  if (str == "Copy") return EventCategory::Copy;
  return EventCategory::Invalid;
}

EventLevel HistoryManager::stringToLevel(const QString &str) {
  if (str == "Warning") return EventLevel::Warning;
  if (str == "Error") return EventLevel::Error;
  if (str == "Info") return EventLevel::Info;
  return EventLevel::Invalid;
}

void HistoryManager::loadHistory() {
  QString path =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QFile file(QDir(path).filePath("history.json"));

  if (!file.exists()) {
    return;
  }

  if (file.open(QIODevice::ReadOnly)) {
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray array = doc.array();
    beginResetModel();
    m_entries.clear();
    for (const auto &val : array) {
      QJsonObject obj = val.toObject();
      EventItem item;
      item.time = QDateTime::fromString(obj["time"].toString(), Qt::ISODate);
      item.category = stringToCategory(obj["category"].toString());
      item.level = stringToLevel(obj["level"].toString());
      item.content = obj["content"].toString();

      // Filter out invalid items if necessary, or keep them.
      // Given we don't care about legacy, we just load what matches our schema.

      m_entries.append(item);
    }
    endResetModel();
  }
}

void HistoryManager::saveHistory() const {
  QString path =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(path);
  if (!dir.exists()) {
    dir.mkpath(".");
  }
  QFile file(dir.filePath("history.json"));
  if (file.open(QIODevice::WriteOnly)) {
    QJsonArray array;
    for (const auto &entry : m_entries) {
      QJsonObject obj;
      obj["time"] = entry.time.toString(Qt::ISODate);
      obj["category"] = categoryToString(entry.category);
      obj["level"] = levelToString(entry.level);
      obj["content"] = entry.content;
      array.append(obj);
    }
    QJsonDocument doc(array);
    file.write(doc.toJson());
  }
}
