#include <QtGui/QtGui>
#include <QtCore/QFile>
#include <QtCore/QStack>
#include "TextEditor.h" // class definition
#include "Highlighter.h"

// ====================================================
//  CTOR
// ====================================================
TextEditor::TextEditor(QWidget* parent)
  : QTextEdit(parent)
{
  mUnsavedChanges = false;

  // Set default format to whatever the first one is
  QStringList formats = config::ConfigFile::instance()->getAllFormatNames();
  if (formats.empty() == false)
    mFormat = formats.first();

  // Create a syntax highlighter for this editor
  mpHighlighter = new Highlighter(document());
  mpHighlighter->setFileFormat(mFormat);

  // Set default number of spaces per tab
  mTabSpaces.fill(' ', 2);

  // Enable drag and drop events
  setAcceptDrops(true);
  setWordWrapMode(QTextOption::NoWrap);

  // Connect some local signals
  connect(this, SIGNAL(textChanged()), this, SLOT(onTextChanged()));
  connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorPositionChanged()));

} // ctor

// ====================================================
//  DTOR
// ====================================================
TextEditor::~TextEditor(void)
{
} // dtor

// ====================================================
//  HAS UNSAVED CHANGES
// ====================================================
bool TextEditor::hasUnsavedChanges() const
{
  return mUnsavedChanges;
} // hasUnsavedChanges

// ====================================================
//  SET TAB SPACES
// ====================================================
void TextEditor::setTabSpaces(int spaces)
{
  if (spaces > 0)
    mTabSpaces.fill(' ', spaces);
} // setTabSpaces

// ====================================================
//  LOAD
// ====================================================
bool TextEditor::load(const QString& path)
{
  bool loaded = false;

  QFile file(path);
  if (file.open(QFile::ReadOnly | QFile::Text))
  {
    // Load the proper format based on the file extension
    QFileInfo fi(file);
    mFormat = config::ConfigFile::instance()->getFormatByExtension(fi.suffix());
    mpHighlighter->setFileFormat(mFormat);

    // Read the file
    setPlainText(file.readAll());
    file.close();

    // This gets set to true when setPlainText() is called because the
    // text changes, but we just loaded the file so there's obviously
    // no unsaved changes.  Reset this flag.
    mUnsavedChanges = false;
    loaded = true;
  }

  return loaded;
} // load

// ====================================================
//  SAVE
// ====================================================
bool TextEditor::save(const QString& path)
{
  bool saved = false;

  QFile file(path);
  if (file.open(QFile::WriteOnly | QFile::Text))
  {
    // Write the file
    file.write(toPlainText().toAscii());
    file.close();

    mUnsavedChanges = false;
    saved = true;
  }

  return saved;
} // save

// ====================================================
//  GET LINE
// ====================================================
QString TextEditor::getLine(int lineNumber) const
{
  QTextCursor cursor(document());

  // Move to the necessary line
  if (lineNumber > 0)
  {
    if (cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, lineNumber) == false)
      return QString::null;
    cursor.movePosition(QTextCursor::StartOfLine);
  }

  cursor.anchor();
  cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
  return cursor.selectedText();
} // getLine

// ====================================================
//  CURRENT LINE NUMBER
// ====================================================
int TextEditor::currentLineNumber(void) const
{
  return toPlainText().left(textCursor().position()).count("\n");
} // currentLineNumber

// ====================================================
//  REPLACE LINE
// ====================================================
bool TextEditor::replaceLine(int lineNumber, const QString& str)
{
  bool replaced = true;

  QTextCursor cursor(document());

  // Move to the necessary line
  bool inPlace = true;
  if (lineNumber > 0)
  {
    if (cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, lineNumber) == false)
      inPlace = false;
    else
      cursor.movePosition(QTextCursor::StartOfLine);
  }

  // If the cursor is in place, select the entire line, remove it, and insert
  // the new line
  if (inPlace)
  {
    cursor.anchor();
    cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(str);

    replaced = true;
  }

  return replaced;
} // replaceLine

// ====================================================
//  LINE NUMBER AT POS
// ====================================================
int TextEditor::lineNumberAtPos(int pos)
{
  return toPlainText().left(pos).count("\n");
} // lineNumberAtPos

// ====================================================
//  SET FILE FORMAT
// ====================================================
void TextEditor::setFileFormat(const QString& format)
{
  if (format != mFormat)
  {
    mFormat = format;
    mpHighlighter->setFileFormat(mFormat);
  }
} // setFileFormat

// ====================================================
//  KEY PRESS EVENT (inherited)
// ====================================================
void TextEditor::keyPressEvent(QKeyEvent* event)
{
  int k = event->key();
  
  // Enter (Auto Indent)
  if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
  {
    // If Ctrl+Enter is pressed, insert a newline at the beginning of this
    // line and move the cursor up one line.
    if (event->modifiers() & Qt::ControlModifier)
    {
      // Move cursor to the begining of the line
      QTextCursor cursor = textCursor();
      cursor.movePosition(QTextCursor::StartOfLine);
      setTextCursor(cursor);

      // Insert a new line
      insertPlainText("\n");

      // Move cursor up one line
      cursor.movePosition(QTextCursor::Up);
      setTextCursor(cursor);
    }
    else
    {
      // Pass through (normal behavior)
      QTextEdit::keyPressEvent(event);
    }
    
    // Insert appropriate number of spaces
    int numIndent = getNumIndent();
    insertPlainText(QString().fill(' ', numIndent));
  }

  // Tab (but not CTRL+Tab)
  else if (event->key() == Qt::Key_Tab)
  {
    if (!(event->modifiers() & Qt::ControlModifier))
    {
      // If there is a selection highlighted, indent it
      if (textCursor().hasSelection())
          increaseIndent();

      // Insert spaces instead of tabs
      else
        insertPlainText(mTabSpaces);
    }
  }

  // Back Tab (Shift+Tab...handled seperately from Tab by Qt)
  else if (event->key() == Qt::Key_Backtab)
  {
    // If there is a selection highlighted, decrease its indentation
    if (textCursor().hasSelection())
      decreaseIndent();
  }

  // Brace Match
  else if (event->key() == Qt::Key_BraceRight)
  {
    matchBraces();
  }

  // Pass Through
  else
  {
    QTextEdit::keyPressEvent(event);
  }

  // emit signal
  emit keyPressed(event);
} // keyPressEvent

// ====================================================
//  GET NUM INDENT
// ====================================================
int TextEditor::getNumIndent(bool toPrevBraceOnly)
{
  QTextCursor cursor = textCursor();
  int cursorPos = cursor.position();
	int retval = 0;
  QStack<int> openBraces;
	
  // Find the position of the last un-closed left brace {
	QString str = toPlainText();
	for (int i = 0; i < cursorPos; i++)
	{
		char ch = str[i].toAscii();

		// Skip comments
		if (ch == '/' && str[i+1] == '/')
			while (str[i] != '\n' && i < cursorPos) i++;

    // Open brace
		else if (ch == '{')
      openBraces.push(i);

    // Close brace
		else if (ch == '}')
    {
      if (openBraces.empty() == false)
			  openBraces.pop();
    }
	}

  // If there's any open braces left, get the indentation of the last one
  if (openBraces.empty() == false)
  {
    int lastOpenBrace = openBraces.pop();
    int lastNewLine = str.lastIndexOf('\n', lastOpenBrace);

    // Get indentation of the last open brace
    retval = (lastOpenBrace - (lastNewLine+1));

    // If toPrevBraceOnly is false, include the tab indentation
    if (!toPrevBraceOnly)
	    retval += mTabSpaces.length();
  }

  return retval;
} // getNumIndent

// ====================================================
//  MATCH BRACES
// ====================================================
void TextEditor::matchBraces()
{
  // Get the string between the current cursor pos and the previous line.  If it's
  // just whitespace, remove it and auto indent.
  QTextCursor cursor(textCursor());
  QString str = toPlainText();
  int cPos = cursor.position();
  int prevNewLine = str.lastIndexOf('\n', cursor.position()-1);
  if (prevNewLine != -1)
  {
    QString trimmed = str.mid(prevNewLine, cursor.position()-prevNewLine).trimmed();
    if (trimmed.isEmpty())
    {
      // If trimmed is not empty, that means there's something other than whitespace
      // between the current cursor pos and the previous line, so remove it and auto
      // indent.
      cursor.anchor();
      if (cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor))
      {
        cursor.removeSelectedText();
      }
      //cursor.setPosition(lastEndLine+1);
      setTextCursor(cursor);

      int numIndent = getNumIndent(true);
      insertPlainText(QString().fill(' ', numIndent));
    }
  }  
  
  // Insert the closing brace
  insertPlainText("}");
} // matchBraces

// ====================================================
//  CAN INSERT FROM MIME DATA (inherited)
// ====================================================
bool TextEditor::canInsertFromMimeData(const QMimeData* source) const
{
  return source->hasUrls() || QTextEdit::canInsertFromMimeData(source);
} // canInsertFromMimeData

// ====================================================
//  INSERT FROM MIME DATA (inherited)
// ====================================================
void TextEditor::insertFromMimeData(const QMimeData* source)
{
  if (source->hasUrls())
  {
    QList<QUrl> urls = source->urls();
    foreach (QUrl url, urls)
      emit fileDropped(url.toLocalFile());
  }
} // insertFromMimeData

// ====================================================
//  ON TEXT CHANGED (slot)
// ====================================================
void TextEditor::onTextChanged(void)
{
  mUnsavedChanges = true;
} // onTextChanged

// ====================================================
//  ON CURSOR POSITION CHANGED (slot)
// ====================================================
void TextEditor::onCursorPositionChanged(void)
{
  // Get the first word on this line
  QTextCursor cursor = textCursor();
  cursor.movePosition(QTextCursor::StartOfLine);
  int startPos = cursor.position();
  cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

  // If the cursor didn't move, then we need to move to the beginning
  // of the next word
  if (cursor.position() == startPos)
  {
    cursor.movePosition(QTextCursor::NextWord);
    cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
  }
  
  QString keyword = cursor.selectedText();
  
  // If the focused keyword changed, emit keywordChanged() signal
  if (keyword != mFocusedKeyword)
  {
    mFocusedKeyword = keyword;
    emit keywordChanged(mFocusedKeyword, mFormat);
  }

} // onCursorPositionChanged

// ====================================================
//  INCREASE INDENT (slot)
// ====================================================
void TextEditor::increaseIndent(void)
{
  QTextCursor cursor = textCursor();
  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();
  
  // Move the cursor to the start of the selection, and then to the
  // begining of that line
  cursor.setPosition(start);
  cursor.movePosition(QTextCursor::StartOfLine);
  
  // Until we reach the end of the document or past the end of the 
  // selection, insert a tab and then move to the next line
  while (cursor.position() < end && cursor.position() >= 0)
  {
    cursor.insertText(mTabSpaces);
    cursor.movePosition(QTextCursor::EndOfLine);
    cursor.movePosition(QTextCursor::NextCharacter);
    end += mTabSpaces.length();
  }
  
} // increaseIndent

// ====================================================
//  DECREASE INDENT (slot)
// ====================================================
void TextEditor::decreaseIndent(void)
{
  static QChar qCh(' ');
  QTextCursor cursor = textCursor();
  
  // Find out how many lines are selected
  int startPos = cursor.selectionStart();
  int endPos = cursor.selectionEnd();
  int lineStart = lineNumberAtPos(startPos);
  int lineEnd = lineNumberAtPos(endPos);

  for (int i = lineStart; i <= lineEnd; ++i)
  {
    // Get the line
    QString line = getLine(i);

    // Figure out how many spaces to remove
    int length = std::min(mTabSpaces.length(), line.length());
    int spaces = 0;
    for (; spaces < length; ++spaces)
    {
      if (line[spaces].isSpace() == false)
        break;
    }

    // Remove the spaces
    if (spaces > 0)
    {
      line.remove(0, spaces);
      replaceLine(i, line);
    }
  }
} // decreaseIndent