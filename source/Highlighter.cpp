#include <QtGui/QtGui>
#include <QtXml/QtXml>
#include "Highlighter.h"  // class definition
#include "ConfigFile.h"

// ====================================================
//  CTOR
// ====================================================
Highlighter::Highlighter(QTextDocument *parent)
  : QSyntaxHighlighter(parent)
{
  
} // ctor

// ====================================================
//  SET FILE FORMAT
// ====================================================
void Highlighter::setFileFormat(const QString& format)
{
  mHighlightingRules = config::ConfigFile::instance()->getHighlightsByFormat(format);
  rehighlight();
} // setFormat

// ====================================================
//  HIGHLIGHT BLOCK (inherited)
// ====================================================
void Highlighter::highlightBlock(const QString &text)
{
  // Highlighting
  foreach (const config::FormatHighlighting &rule, mHighlightingRules) 
  {
    bool ruleApplied = false;
    foreach (const QRegExp& expression, rule.patterns)
    {
      int index = expression.indexIn(text);
      while (index >= 0) 
      {
        ruleApplied = true;
        int length = expression.matchedLength();
        setFormat(index, length, rule.format);
        index = expression.indexIn(text, index + length);
      }
    }

    if ((rule.name == "comment" || rule.name == "string") && ruleApplied)
      return;
  }

  setCurrentBlockState(0);
} // highlightBlock