#include <QtGui/QtGui>
#include "MainWindow.h" // class definition
#include "IDE.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  setupEditor();
  setupFileMenu();
  setupOptionsMenu();
  setupHelpMenu();
  
  setWindowTitle(tr("Material IDE"));
}

void MainWindow::about()
{
  QMessageBox::about(this, "Material IDE", "No About");
}

void MainWindow::setupEditor()
{
  mpIde = new IDE(this);
  setCentralWidget(mpIde);
}

void MainWindow::setupFileMenu()
{
  QMenu* fileMenu = new QMenu("&File", this);
  menuBar()->addMenu(fileMenu);

  fileMenu->addAction("&New", mpIde, SLOT(newFile()), QKeySequence::New);
  fileMenu->addAction("&Open", mpIde, SLOT(open()), QKeySequence::Open);
  fileMenu->addAction("&Save", mpIde, SLOT(save()), QKeySequence::Save);
  fileMenu->addAction("Save &As", mpIde, SLOT(saveAs()), QKeySequence::SaveAs);
  fileMenu->addSeparator();
  fileMenu->addAction("E&xit", qApp, SLOT(quit()), QKeySequence::Close);
}

void MainWindow::setupOptionsMenu()
{
  QMenu* optMenu = new QMenu("&Options", this);
  menuBar()->addMenu(optMenu);

  QMenu* formatMenu = new QMenu("Change Format", optMenu);
  connect(formatMenu, SIGNAL(triggered(QAction*)), this, SLOT(temp(QAction*)));
  optMenu->addMenu(formatMenu);

  // Add a menu option for all supported formats
  QStringList formats = config::ConfigFile::instance()->getAllFormatNames();
  foreach (QString format, formats)
    formatMenu->addAction(format);
}

void MainWindow::setupHelpMenu()
{
  QMenu *helpMenu = new QMenu(tr("&Help"), this);
  menuBar()->addMenu(helpMenu);

  helpMenu->addAction(tr("&About"), this, SLOT(about()));
  helpMenu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
}

void MainWindow::temp(QAction* act)
{
  mpIde->setCurrentFormat(act->text());
}