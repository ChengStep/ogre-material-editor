#ifndef _DEBUG
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif // WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#endif

#include <QtGui/QApplication>
#include <QtGui/qmessagebox.h>
#include "MainWindow.h"

int main(int argc, char** argv);

#ifndef _DEBUG
INT __stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR cmdLine, INT)
{
  QStringList filesWithSpaces = QString(cmdLine).split(QRegExp("\".*\""), QString::SkipEmptyParts);
  QStringList files;
  foreach (QString file, filesWithSpaces)
    files += file.split(" ", QString::SkipEmptyParts);

  QString blah;
  foreach (QString file, files)
    blah += file + "\n";

  //QMessageBox::information(NULL, "blah", blah);

  int argc = 2;
  char* argv[2] = {"Material Editor.exe", cmdLine};
  return main(argc, argv);
}
#endif

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  MainWindow window;
  window.resize(640, 512);
  window.show();
  return app.exec();
}
