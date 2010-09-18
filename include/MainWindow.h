#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_
#include <QtGui/QMainWindow>
#include "Highlighter.h"

class IDE;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = 0);

public slots:
  void about();
  void temp(QAction*);
  
private:
  void setupEditor();
  void setupFileMenu();
  void setupOptionsMenu();
  void setupHelpMenu();

  IDE* mpIde;
};

#endif // _MAINWINDOW_H_