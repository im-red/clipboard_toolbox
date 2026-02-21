#include <QApplication>
#include <QIcon>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  qSetMessagePattern(
      "%{time yyyy-MM-dd h:mm:ss.zzz} [%{type}] (%{file}:%{line}) %{function} "
      "- %{message}");
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  QCoreApplication::setOrganizationName("imred");
  QCoreApplication::setApplicationName("Clipboard Toolbox");

  QIcon icon(":/icon.png");
  app.setWindowIcon(icon);

  MainWindow window;
  window.show();

  return app.exec();
}
