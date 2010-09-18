#ifndef _CONFIGFILE_H_
#define _CONFIGFILE_H_
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtGui/QTextFormat>
#include <QtGui/QColor>

namespace config
{
  // FORWARD DECLARATIONS
  struct FormatHighlighting;
  struct FormatWord;
  typedef QMap<QString, FormatHighlighting> FormatHighlightingMap;
  typedef QMap<QString, FormatWord> FormatWordMap;

  /**
   */
  class ConfigFile
  {
  public:  
    /// @returns A static pointer to the ConfigFile singleton.
    static ConfigFile* instance();

    /// @returns The path to the Ogre Manual.
    const QString& getManualPath(void) const;

    /** @param The file extension.  Should not include a "."
     * @returns The supported format for the given file extension, or 
     *       empty string if there is no known format for that extension. */
    const QString& getFormatByExtension(const QString& extension) const;

    /** @param format The format whose highlights you want to get.
     * @returns A map of all of the FormatHighlightings for this format, or an 
     *      empty map if the format is not supported. */
    const FormatHighlightingMap& getHighlightsByFormat(const QString& format) const;

    /** @param format The format whose words you want to get.
     * @returns A map of all of the FormatWords for this format, or an empty
     *     map if the format is not supported. */
    const FormatWordMap& getWordsByFormat(const QString& format) const;

    /** @returns a QStringList containing all valid format names. */
    QStringList getAllFormatNames(void) const;

  private:
    /// Private ctor
    ConfigFile(void);

    /// Private dtor
    ~ConfigFile(void);

    /// Load the config file
    void load(void);

    /// Load a particular format
    void loadFormat(const QString&, const QString&, const QString&);

  private:
    static ConfigFile*                    mpMe;
    QString                               mManualPath;
    QMap<QString, FormatHighlightingMap>  mHighlightsByFormat;
    QMap<QString, FormatWordMap>          mWordsByFormat;
    QMap<QString, QString>                mFormatsByExt;
  };


  /** Contains details about how to highlight a type of word or
   * regular expression, such as a keyword or comment.  These 
   * details are used by the syntax highlighter to know how to
   * highlight different parts of a document based on the format. */
  struct FormatHighlighting
  {
    /// Name of the highlighting rule
    QString name;

    /// Paterns that match this highlighting rule
    QVector<QRegExp> patterns;

    /// Format for this rule
    QTextCharFormat format;
  };

  /** Contains details about a word that is recognized in a particular 
   * format as one that should be highlighted.  This is used by the
   * syntax highlighter and IDE to know how to highlight the word and 
   * to lookup the documentation for that word, if it exists. */
  struct FormatWord
  {
    /// The word that should be highlighted
    QString word;

    /// The type of highlighting that applies to this word
    QString highlightType;

    /// Documentation file where this word is defined (if it exists).
    QString doc;
  };
}

#endif // _CONFIGFILE_H_