//#ifdef Q_WS_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinBase.h>
//#endif // Q_WS_WIN

#include <QtGui/QtGui>
#include "IDE.h"  // class declarations
#include "TextEditor.h"
#include "ConfigFile.h"

IDE::FileEditor::FileEditor() {editor = NULL;}
IDE::FileEditor::~FileEditor() {if (editor) {editor->setUserData(0, NULL); delete editor; editor = NULL;}}

// ====================================================
//  CTOR
// ====================================================
IDE::IDE(QWidget* parent, bool statusBar)
  : QWidget(parent)
{
  setAcceptDrops(true);
  mKeywordSyntaxesItr = mKeywordSyntaxes.end();

  // Load the ConfigFile
  config::ConfigFile::instance();

  // Create a VBox Layout for the editors and status bar
  QVBoxLayout* vbox = new QVBoxLayout;
  vbox->setMargin(0);
  vbox->setSpacing(0);

  // Define the default font for each editor
  mFont;
  mFont.setFamily("Courier New");
  mFont.setFixedPitch(true);
  mFont.setPointSize(10);

  // Create the tab widget that will hold each editor.
  mpTabs = new QTabWidget(this);
  mpTabs->setTabsClosable(true);
  mpTabs->setMovable(true);
  mpTabs->setTabShape(QTabWidget::Triangular);
  connect(mpTabs, SIGNAL(currentChanged(int)), this, SLOT(onTabChanged(int)));
  connect(mpTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(onTabCloseRequested(int)));
  
  // Create the status bar.  Only show it if statusBar is true
  mpStatusBar = new QStatusBar(this);
  mpStatusBar->setVisible(statusBar);
    
  // Add the tab widget and status bar to the vbox
  vbox->addWidget(mpTabs);
  vbox->addWidget(mpStatusBar);

  setLayout(vbox);
} // ctor

// ====================================================
//  DTOR
// ====================================================
IDE::~IDE(void)
{
  foreach (FileEditor* fe, mEditors)
    delete fe;
} // dtor

// ====================================================
//  ADD EDITOR
// ====================================================
void IDE::addEditor(const QString& filename, const QString& path)
{
  // Create a new FileEditor struct to hold the filename, path, and
  // TextEditor for the new editor
  FileEditor* fe = new FileEditor;
  fe->filename = filename;
  fe->path = path;
  fe->editor = new TextEditor(mpTabs);
  fe->editor->setFont(mFont);
  fe->editor->setUserData(0, fe);
  connect(fe->editor, SIGNAL(fileDropped(const QString&)), this, SLOT(onFileDropped(const QString&)));
  connect(fe->editor, SIGNAL(keywordChanged(const QString&, const QString&)), this, SLOT(onKeywordChanged(const QString&, const QString&)));
  connect(fe->editor, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(onEditorKeyEvent(QKeyEvent*)));
  mEditors.push_back(fe);

  // Add this new editor to the tab widget and make it the current tab
  int tab = mpTabs->addTab(fe->editor, fe->filename);
  mpTabs->setTabToolTip(tab, fe->path);
  mpTabs->setCurrentWidget(fe->editor); // causes mpTabs to emit signal currentChanged()
} // addEditor

// ====================================================
//  DRAG ENTER EVENT (inherited)
// ====================================================
void IDE::dragEnterEvent(QDragEnterEvent* e)
{
  if (e->mimeData()->hasUrls())
    e->acceptProposedAction();
  else
    QWidget::dragEnterEvent(e);
} // dragEnterEvent

// ====================================================
//  DROP EVENT (inherited)
// ====================================================
void IDE::dropEvent(QDropEvent* e)
{
  if (e->mimeData()->hasUrls())
  {
    QList<QUrl> urls = e->mimeData()->urls();
    foreach (QUrl url, urls)
      onFileDropped(url.toLocalFile());
  }
  else
    QWidget::dropEvent(e);
} // dropEvent

// ====================================================
//  SET KEYWORD
// ====================================================
void IDE::setKeyword(const QString& keyword, const QString& format)
{
  const config::FormatWordMap& words = 
    config::ConfigFile::instance()->getWordsByFormat(format);

  // Clear the current status
  mpStatusBar->clearMessage();

  // Clear any existing syntaxes
  mKeywordSyntaxes.clear();
  
  config::FormatWordMap::const_iterator citr = words.find(keyword);
  if (citr != words.end())
  {
    QString docFile = citr->doc;
    const QString& manualPath = config::ConfigFile::instance()->getManualPath();
      
    QFile file(manualPath + docFile);
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
      QTextStream stream(&file);
      QString text = stream.readAll();
      file.close();

      
      // Probably never going to be 4 different formats, but just to be sure...
      for (int i = 0; i < 4; ++i)
      {
        // Look for the string "Format: <keyword>"
        QString pattern = QString("Format%1: %2 ")
                          .arg((i==0) ? "" : QString::number(i))
                          .arg(keyword);
        QStringMatcher matcher(pattern);
        int index = matcher.indexIn(text);
        if (index >= 0)
        {
          // Look for the first <BR> after the format.
          matcher.setPattern("<BR>");
          int endIndex = matcher.indexIn(text, index);
          if (endIndex >= 0)
          {
            // The syntax is between the two indices.
            QString syntax = text.mid(index, endIndex-index);

            // Format the synatx properly
            syntax.replace("&lt;", "<");
            syntax.replace("&gt;", ">");

            // Add to the keyword syntax vector
            mKeywordSyntaxes.push_back(syntax);
          }
        }
      }
    }
  }

  // Point to the first syntax
  mKeywordSyntaxesItr = mKeywordSyntaxes.begin();

  // If there is at least one valid syntax, show the first one
  if (mKeywordSyntaxesItr != mKeywordSyntaxes.end())
    mpStatusBar->showMessage(*mKeywordSyntaxesItr);
} // setKeyword

// ---------------------------------------------------------------------
//                               SLOTS
// ---------------------------------------------------------------------

// ====================================================
//  ON FILE DROPPED (slot)
// ====================================================
void IDE::onFileDropped(const QString& path)
{
  QFileInfo fi(path);
  if (fi.exists())
  {
    // See if a text editor for this file already exists
    foreach (FileEditor* fe, mEditors)
    {
      if (fe->path == fi.absoluteFilePath())
      {
        mpTabs->setCurrentWidget(fe->editor);
        return;
      }
    }

    QString filename = fi.fileName();
    QString path = fi.absoluteFilePath();

    addEditor(filename, path);
    mpCurrentEditor->editor->load(path);
  }
} // onFileDropped

// ====================================================
//  NEW FILE (slot)
// ====================================================
void IDE::newFile(void)
{
  addEditor("untitled", "");
} // neFile

// ====================================================
//  SAVE (slot)
// ====================================================
void IDE::save(void)
{
  if (mpCurrentEditor)
  {
    // If the path for this file exists, save to it
    if (QFile::exists(mpCurrentEditor->path))
      mpCurrentEditor->editor->save(mpCurrentEditor->path);

    // Otherwise, call saveAs()
    else
      saveAs();
  }
} // save

// ====================================================
//  SAVE AS (slot)
// ====================================================
void IDE::saveAs(void)
{
  if (mpCurrentEditor)
  {
    QString path = QFileDialog::getSaveFileName(this, "Save As");
    if (!path.isNull())
    {
      QFileInfo fi(path);
      mpCurrentEditor->editor->save(path);
      mpCurrentEditor->path = fi.absoluteFilePath();
      mpCurrentEditor->filename = fi.fileName();
      mpTabs->setTabText(mpTabs->indexOf(mpCurrentEditor->editor), mpCurrentEditor->filename);
    }
  }
} // saveAs

// ====================================================
//  OPEN (slot)
// ====================================================
void IDE::open(void)
{
  QStringList files = QFileDialog::getOpenFileNames(this, "Open Files");
  foreach (QString path, files)
  {
    QFileInfo fi(path);
    addEditor(fi.fileName(), fi.absoluteFilePath());
    mpCurrentEditor->editor->load(path);
  }
} // open

// ====================================================
//  ON TAB CHANGED (slot)
// ====================================================
void IDE::onTabChanged(int tab)
{
  if (mpTabs->currentWidget())
  {
    mpCurrentEditor = static_cast<FileEditor*>(mpTabs->currentWidget()->userData(0));
    mpCurrentEditor->editor->setFocus();
  }
  else
  {
    mpCurrentEditor = NULL;
  }
} // onTabChanged

// ====================================================
//  ON TAB CLOSE REQUESTED (slot)
// ====================================================
void IDE::onTabCloseRequested(int tab)
{
  bool discard = false;

  FileEditor* editor = static_cast<FileEditor*>(mpTabs->widget(tab)->userData(0));
  if (editor->editor->hasUnsavedChanges())
  {
    int r = QMessageBox::warning(this, "Unsaved Changes", QString("Do you want to save changes to %1 before closing?").arg(editor->filename),
                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    
    if (r == QMessageBox::Save) 
    {
      FileEditor* current = mpCurrentEditor;
      mpCurrentEditor = editor;
      save();
      mpCurrentEditor = current;
    }
    else if (r == QMessageBox::Discard) discard = true;
  }

  // If the editor now has no unsaved changes, close it.  This isn't an else 
  // because the above if statement may have been true and the contents of the
  // editor saved.  We ONLY want to close the tab if the contents were saved, or
  // the operator wants to discard changes.
  if (!editor->editor->hasUnsavedChanges() || discard)
  {
    // Save the current editor before we remove it, because once it's removed then
    // mpCurrentEditor points to something else.
    FileEditor* old = editor;

    // Remove the current tab
    mpTabs->removeTab(tab);

    // Remove the current editor from the vector
    QVector<FileEditor*>::iterator itr = mEditors.begin();
    while (itr != mEditors.end() && (*itr) != old) ++itr;
    if (itr != mEditors.end())
      mEditors.erase(itr);

    // Delete the removed editor
    delete old;
  }
} // onTabCloseRequested

// ====================================================
//  ON KEYWORD CHANGED (slot)
// ====================================================
void IDE::onKeywordChanged(const QString& keyword, const QString& format)
{
  // See if this is a valid keyword for this format
  const config::FormatWordMap& words = 
    config::ConfigFile::instance()->getWordsByFormat(format);
  
  setKeyword(keyword, format);
} // onKeywordChanged

// ====================================================
//  ON EDITOR KEY EVENT (slot)
// ====================================================
void IDE::onEditorKeyEvent(QKeyEvent* e)
{
  // CTRL+Up or CTRL+Down : Change which keyword syntax is displayed
  if (e->modifiers() & Qt::ControlModifier &&
      mKeywordSyntaxesItr != mKeywordSyntaxes.end())
  {
    if (e->key() == Qt::Key_Up)
    {
      // Advance the iterator
      ++mKeywordSyntaxesItr;
      if (mKeywordSyntaxesItr == mKeywordSyntaxes.end())
        mKeywordSyntaxesItr = mKeywordSyntaxes.begin();
    }
    else if (e->key() == Qt::Key_Down)
    {
      // Decrease the iterator
      if (mKeywordSyntaxesItr != mKeywordSyntaxes.begin())
        --mKeywordSyntaxesItr;
      else
        mKeywordSyntaxesItr = (mKeywordSyntaxes.end() - 1);
    }

    mpStatusBar->showMessage(*mKeywordSyntaxesItr);
  }

  // CTRL+Tab : Select next tab
  else if (e->modifiers() & Qt::ControlModifier && e->key() == Qt::Key_Tab)
  {
    int index = mpTabs->currentIndex() + 1;
    if (index == mpTabs->count())
      index = 0;
    mpTabs->setCurrentIndex(index);
  }

  // CTRL+BackTab (SHIFT+Tab) : Select prev tab
  else if (e->modifiers() & Qt::ControlModifier && e->key() == Qt::Key_Backtab)
  {
    int index = mpTabs->currentIndex() - 1;
    if (index < 0)
      index = mpTabs->count() - 1;
    mpTabs->setCurrentIndex(index);
  }
} // onEditorKeyEvent

// ====================================================
//  SET CURRENT FORMAT (slot)
// ====================================================
void IDE::setCurrentFormat(const QString& format)
{
  if (mpCurrentEditor)
    mpCurrentEditor->editor->setFileFormat(format);
} // setCurrentFormat