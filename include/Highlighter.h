#ifndef _HIGHLIGHTER_H_
#define _HIGHLIGHTER_H_
#include <QtGui/QSyntaxHighlighter>
#include <QtCore/QHash>
#include <QtGui/QTextCharFormat>
#include "ConfigFile.h"

// FORWARD DECLARATIONS
class QTextDocument;

class Highlighter : public QSyntaxHighlighter
{
  Q_OBJECT

public:
  Highlighter(QTextDocument *parent = NULL);
  void setFileFormat(const QString& format);

protected:
  void highlightBlock(const QString &text);
  void loadRulesSettings(void);
  void loadRule(const QString& ruleName);

private:
  config::FormatHighlightingMap mHighlightingRules;
};

#endif // _HIGHLIGHTER_H_