#ifndef _IDE_H_
#define _IDE_H_
#include <QtGui/QWidget>

// FORWARD DECLARATIONS
class QTabWidget;
class QStatusBar;
class TextEditor;

class IDE : public QWidget
{
  Q_OBJECT

public:
  IDE(QWidget* parent = NULL, bool statusBar = true);
  ~IDE(void);

public slots:
  void newFile(void);
  void save(void);
  void saveAs(void);
  void open(void);
  void setCurrentFormat(const QString& format);

protected:
  void addEditor(const QString& filename, const QString& path);
  void dragEnterEvent(QDragEnterEvent*);
  void dropEvent(QDropEvent*);
  void setKeyword(const QString& keyword, const QString& format);

protected slots:
  void onFileDropped(const QString&);
  void onTabChanged(int);
  void onTabCloseRequested(int);
  void onKeywordChanged(const QString&, const QString&);
  void onEditorKeyEvent(QKeyEvent*);

protected:
  struct FileEditor : public QObjectUserData
  {
    FileEditor();
    ~FileEditor();

    QString filename;
    QString path;
    TextEditor* editor;
  };

  QFont                 mFont;
  QTabWidget*           mpTabs;
  QStatusBar*           mpStatusBar;
  QVector<FileEditor*>  mEditors;
  QVector<QString>      mKeywordSyntaxes;
  QVector<QString>::iterator mKeywordSyntaxesItr;
  FileEditor*           mpCurrentEditor;
};

#endif // #endif