#pragma once

#include <QDateTime>
#include <QString>

enum class EventCategory { Invalid = -1, Copy, AutoSaveImage };

enum class EventLevel { Invalid = -1, Info, Warning, Error };

struct EventItem {
  QDateTime time;
  EventCategory category;
  EventLevel level;
  QString content;
};
