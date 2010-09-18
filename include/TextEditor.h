#ifndef _TEXTEDITOR_H_
#define _TEXTEDITOR_H_
#include <QtGui/QTextEdit>

// FORWARD DECLARATIONS
class Highlighter;

class TextEditor : public QTextEdit
{
  Q_OBJECT

public:
  /** Default CTOR */
  TextEditor(QWidget* parent = NULL);

  /** Default DTOR */
  ~TextEditor(void);

  /** Sets the number of spaces that get inserted whenever TAB is pressed.
   * @param spaces The number of spaces to insert. */
  void setTabSpaces(int spaces);

  /** @returns TRUE if the document has unsaved changes, FALSE otherwise. */
  bool hasUnsavedChanges() const;

  /** Load the contents of a file into the TextEditor.
   * @param path The path of the file to load.
   * @returns TRUE if the file is successfuly loaded, FALSE otherwise. */
  bool load(const QString& path);

  /** Saves the contents of the TextEditor to file.
   * @param path The path of the file to save to.
   * @param TRUE if the file is successfuly saved, FALSE otherwise. */
  bool save(const QString& path);

  /** @param lineNumber The number of the line to return.
   * @returns The text on the specified line number, or a null QString if the
   *          line does not exist in the TextEditor. */
  QString getLine(int lineNumber) const;

  /** @returns The current line number.  This can be used with getLine() to get
   * the current line. */
  int currentLineNumber(void) const;

  /** Replace a line of text with a new string.
   * @param lineNumber The number of the line to replace.  All text on this 
   *        line will be replaced by the new string.
   * @param str The string to replace the line with.
   * @returns TRUE if the line is replaced, FALSE otherwise.  For example, 
   *        passing an invalid line number will cause this to return FALSE. */
  bool replaceLine(int lineNumber, const QString& str);

  /** @param pos The desired cursor position.
   * @returns The line number that cursor position \e pos lies on. */
  int lineNumberAtPos(int pos);

  /** Sets the current file format to format.  This can be used to manually 
   * specify the format for a file you're working on if it either wasn't 
   * recognized when loaded, or you started editing a new file.
   * @param format The desired file format. */
  void setFileFormat(const QString& format);

public slots:
  /** Increases the indentation of all selected lines.  The size of the 
   * indentation is determined by the current number of tab spaces. */
  void increaseIndent(void);

  /** Decreases the indentation of all selected lines.  The size of the
   * indentation is determined by the current number of tab spaces. */
  void decreaseIndent(void);

signals:
  /** Emitted whenever a file is dropped onto the TextEditor.  The TextEditor
   * class does not handle file drops directly, but rather expects some other
   * widget to handle it by either telling the TextEditor to load the file,
   * or creating a new TextEditor to load the file instead.
   * @param path The full path of the file dropped onto the TextEditor. */
  void fileDropped(const QString& path);

  /** Emitted whenever the focused keyword changes.  A keyword stays in focus
   * as long as you are on the same line as it, so the basically is emitted
   * whenever you move the cursor to a line with a different keyword, or the 
   * keyword on this line changes.
   * The word emitted in this signal may be an empty string if the current line
   * has no keyword.
   * @param keyword The new keyword in focus.  This could be an empty string if
   *        the current line has no keyword.
   * @param format The format that this keyword applies to. */
  void keywordChanged(const QString& keyword, const QString& format);

  /** Emitted whenever a key is pressed in the editor.  This allows the any 
   * other widgets to know about key press events received by the editor so
   * that they may respond to them as well.  For example, IDE listens for
   * CTRL+Up and CTRL+Down key events to change the displayed syntax for a 
   * keyword.
   * @param event The QKeyEvent received in keyPressEvent(). */
  void keyPressed(QKeyEvent* event);

protected:
  /** Handle special case key events to produce things such as auto-indentation
   * and brace matching.  All other key events are passed through to the base
   * class. */
  void keyPressEvent(QKeyEvent*);

  /** Gets the number of spaces that the current line should be indented to 
   * match the indentation of its scope.
   * @param toPrevBraceOnly If TRUE, this will only return how many spaces the
   *        opening brace for this scope is indented.  Otherwise, this returns
   *        the number of spaces that a new line in this scope should be 
   *        indented.
   * @returns The number of spaces that the current line should be indented. */
  int getNumIndent(bool toPrevBraceOnly = false);

  /** Will insert a closing brace } that lines up with the most recent open
   * brace {.  If there are no open braces, this will insert a closing brace at
   * the begining of the line.  If there is text between the beginning of the
   * line and the cursor, the closing brace will be inserted at the current
   * cursor position. */
  void matchBraces(void);

  /** Used to determine if the MIME data contains urls to handle file drops.
   * @param source The MIME data that is being drug onto the widget.
   * @returns TRUE if the source contains urls or any other supported MIME
   *          type, FALSE otherwise. */
  bool canInsertFromMimeData(const QMimeData* source) const;

  /** Used to receive drop event from file drops.  All other MIME sources
   * are handled by the base class.  When a file is dropped onto the 
   * TextEditor, this method will emit the fileDropped() signal.
   * @param source The MIME data that is being dropped onto the widget. */
  void insertFromMimeData(const QMimeData* source);

protected slots:
  /** Marks the contents of the TextEditor as having unsaved changes. */
  void onTextChanged(void);
  
  /** Checks the for a new focused keyword */
  void onCursorPositionChanged(void);

protected:
  bool          mUnsavedChanges;
  QString       mFormat;
  QString       mTabSpaces;
  QString       mFocusedKeyword;
  Highlighter*  mpHighlighter;
};

#endif // _TEXTEDITOR_H_