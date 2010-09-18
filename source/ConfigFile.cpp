#include "ConfigFile.h"
#include <QtXml/QtXml>

namespace config
{
  // Initialize Static Members
  ConfigFile* ConfigFile::mpMe = NULL;

  // ====================================================
  //  CTOR
  // ====================================================
  ConfigFile::ConfigFile(void)
  {
    load();
  } // ctor

  // ====================================================
  //  DTOR
  // ====================================================
  ConfigFile::~ConfigFile(void)
  {
  } // dtor

  // ====================================================
  //  INSTANCE (static)
  // ====================================================
  ConfigFile* ConfigFile::instance()
  {
    if (mpMe == NULL)
      mpMe = new ConfigFile;
    return mpMe;
  } // instance

  // ====================================================
  //  GET MANUAL PATH
  // ====================================================
  const QString& ConfigFile::getManualPath(void) const
  {
    return mManualPath;
  } // getManualPath

  // ====================================================
  //  GET FORMAT BY EXTENSION
  // ====================================================
  const QString& ConfigFile::getFormatByExtension(const QString& extension) const
  {
    static QString FORMAT_NOT_FOUND;

    QMap<QString, QString>::const_iterator citr = mFormatsByExt.find(extension);
    return (citr != mFormatsByExt.end() ? (*citr) : FORMAT_NOT_FOUND);
  } // getFormatByExtension

  // ====================================================
  //  GET HIGHLIGHTS BY FORMAT
  // ====================================================
  const FormatHighlightingMap& ConfigFile::getHighlightsByFormat(const QString& format) const
  {
    static const FormatHighlightingMap FORMAT_NOT_FOUND;

    if (format.isNull())
      return FORMAT_NOT_FOUND;

    QMap<QString, FormatHighlightingMap>::const_iterator citr = mHighlightsByFormat.find(format);
    return (citr != mHighlightsByFormat.end() ? (*citr) : FORMAT_NOT_FOUND);
  } // getHighlightsByFormat

  // ====================================================
  //  GET WORDS BY FORMAT
  // ====================================================
  const FormatWordMap& ConfigFile::getWordsByFormat(const QString& format) const
  {
    static const FormatWordMap FORMAT_NOT_FOUND;

    if (format.isNull())
      return FORMAT_NOT_FOUND;

    QMap<QString, FormatWordMap>::const_iterator citr = mWordsByFormat.find(format);
    return (citr != mWordsByFormat.end() ? (*citr) : FORMAT_NOT_FOUND);
  } // getWordsByFormat

  // ====================================================
  //  GET ALL FORMAT NAMES
  // ====================================================
  QStringList ConfigFile::getAllFormatNames(void) const
  {
    QStringList formats;

    QMap<QString, QString>::const_iterator citr = mFormatsByExt.begin();
    for (; citr != mFormatsByExt.end(); ++citr)
      formats << (*citr);

    return formats;
  } // getAllFormatNames

  // ====================================================
  //  LOAD
  // ====================================================
  void ConfigFile::load(void)
  {
    QFile file("config.xml");
    if (file.open(QFile::ReadOnly | QFile::Text))
    {
      QDomDocument doc;
      if (doc.setContent(&file))
      {
        QDomElement root = doc.documentElement();

        // Manual Path
        QDomElement manPath = root.firstChildElement("OgreManualPath");
        if (manPath.isNull() == false)
        {
          mManualPath = manPath.text() + "\\";  // Make sure it ends with a /
        }

        // Formats (required)
        QDomElement formatsNode = root.firstChildElement("Formats");
        QDomNodeList formats = root.elementsByTagName("Format");

        for (int i = 0; i < formats.count(); ++i)
        {
          QDomElement child = formats.item(i).toElement();
          QString formatName = child.text();
          QString highFile   = child.attribute("highlights_file");
          QString wordFile   = child.attribute("words_file");
          QString fileExt    = child.attribute("file_extensions");

          // Load this format
          loadFormat(formatName, highFile, wordFile);

          // Associate this format with its file extension
          mFormatsByExt[fileExt] = formatName;
        }
      }
    }
  } // load

  // ====================================================
  //  LOAD FORMAT
  // ====================================================
  void ConfigFile::loadFormat(const QString& formatName, const QString& highlightsFile, const QString& wordsFile)
  {    
    FormatHighlightingMap highlightsMap;
    FormatWordMap wordsMap;
    
    // Highlight rules
    {
      QFile file(highlightsFile);
      if (file.open(QFile::ReadOnly | QFile::Text))
      {
        QDomDocument doc;
        if (doc.setContent(&file))
        {
          QDomElement root = doc.documentElement();
          QDomNodeList highlights = root.elementsByTagName("Highlight");
        
          for (int i = 0; i < highlights.count(); ++i)
          {
            FormatHighlighting rule;
            QDomElement child = highlights.item(i).toElement();
            rule.name = child.attribute("name");
            rule.format.setForeground(QColor(child.attribute("color", "#000000")));
            bool bold = (child.attribute("bold", "false").toLower() == "true");
            bool italics = (child.attribute("italics", "false").toLower() == "true");
          
            if (bold)    rule.format.setFontWeight(QFont::Bold);
            if (italics) rule.format.setFontItalic(true);

            if (rule.name == "comment")
            {
              rule.patterns.clear();
              rule.patterns.append(QRegExp("//[^\n]*"));
            }
            else if (rule.name == "string")
            {
              rule.patterns.clear();
              rule.patterns.append(QRegExp("\".*\""));
            }
            
            // Add this highlighting rule to the map
            highlightsMap[rule.name] = rule;
          }
        }
      }
    }

    // Words
    {
      QFile file(wordsFile);
      if (file.open(QFile::ReadOnly | QFile::Text))
      {
        QDomDocument doc;
        if (doc.setContent(&file))
        {
          QDomElement root = doc.documentElement();
          QDomNodeList words = root.elementsByTagName("Word");
        
          for (int i = 0; i < words.count(); ++i)
          {
            QDomElement child = words.item(i).toElement();
            FormatWord formatWord;
            formatWord.highlightType = child.attribute("highlight");
            formatWord.doc           = child.attribute("documentation");
            formatWord.word          = child.text();

            // Add this word to the highlighting rules map under its highlight category (ie, keywords)
            QMap<QString, FormatHighlighting>::Iterator highItr = highlightsMap.find(formatWord.highlightType);
            if (highItr != highlightsMap.end())
            {
              // Add this word to the patterns for its highlight type
              highItr->patterns.append(QRegExp(QString("\\b%1\\b").arg(formatWord.word)));

              // Add this word to the format words for this format
              wordsMap[formatWord.word] = formatWord;               
            }
          }
        }
      }
    }

    // Add this FormatHighlightMap to the map of formats
    if (highlightsMap.empty() == false)
      mHighlightsByFormat[formatName] = highlightsMap;

    // Add this FormatWordMap to the map of formats
    if (wordsMap.empty() == false)
      mWordsByFormat[formatName] = wordsMap;
  } // loadFormat
} // namespace config