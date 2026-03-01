#include "utils.h"

namespace utils {

QString formatSize(qint64 bytes) {
  if (bytes >= 1024 * 1024) {
    return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
  } else if (bytes >= 1024) {
    return QString::number(bytes / 1024.0, 'f', 2) + " KB";
  } else {
    return QString::number(bytes) + " B";
  }
}

}
