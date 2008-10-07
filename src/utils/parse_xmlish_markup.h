/**
 * @author George Foster
 * @file parse_xmlish_markup.h  Parse XML-like markup
 * 
 * COMMENTS: 
 *
 * The key part is "ish"; this makes no guarantees. What it knows about is:
 *
 * @verbatim
 * <?xml declaration...?>
 * <!-- comment -->
 * <normaltag attr1="val1">bla bla</normaltag>
 * <normaltag attr1="val1"/>
 * <![CDATA[ .... ]]>
 * @endverbatim
 *
 * Known departures from XML:
 * - It's permissive about what appears within attribute value quotes,
 *   allowing anything except quotes of the same type.
 * - No <!DOCTYPE...> tags
 * + lots of unknowns...
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */
#ifndef PARSE_XMLISH_MARKUP_H
#define PARSE_XMLISH_MARKUP_H

#include <string>
#include <vector>
#include "portage_defs.h"

namespace Portage {

/// One XML "ish" Tag.
struct XMLishTag 
{
   /// tag types: <...>  <?...?> <!--...--> <![CDATA[...]]> <!DOCTYPE ...>:
   enum {isElement, isDecl, isComment, isCDATA, isDoctype} tag_type;

   string name;                ///< the tags's name (or its content if comment or cdata).
   vector< pair<string,string> > attr_vals; ///< list of attribute/value pairs.
   bool is_beg_tag;             ///< "<bla ...>" or "<bla .../>"
   bool is_end_tag;             ///< "<bla .../>" or "</bla>"

   /// Clears the XML tag.
   void clear() {name.clear(); attr_vals.clear(); is_beg_tag = is_beg_tag = false;}

   /// Appends the XML tag attributs to s.
   /// @param s string to which to append the attributs.
   /// @return Returns s
   string& attrValsString(string& s);

   /// Checks value of an XML tag attribute
   /// @param s attribute name
   /// @returns value as string (empty if this attribute is not existant
   string attrVal(const string &s);

   /// Format the XML Tag to a string.
   /// @param s string in which to format the XML tag
   /// @return Returns s.
   string& toString(string& s);

   /// Format the XML Tag to a string.
   /// @return Returns a string containing the XML tag.
   string toString() {string s; return toString(s);}
};

/**
 * Parse next XML tag in a buffer. Aborts with an error message if it
 * encounters really malformed XML, but returns with no error if it encounters
 * the end of the buffer while parsing a tag.
 *
 * @param buf  string to work on.
 * @param tag  the parsed tag
 * @param beg  if not NULL, *beg is set to offset of starting "<", or end of buf if
 *             no tag found.
 * @param end  if not NULL, *end is set to offset+1 of ending ">".
 * @return     status: true iff a tag found. If only a partial tag was found, return
 *             false, but with beg set to the initial "<" instead of to end of buffer;
 *             contents of tag and end are undefined in this case.
 */
bool parseXMLishTag(const char buf[], XMLishTag& tag, Uint *beg, Uint* end);

/**
 * Replace XML special characters with escape codes.
 * @param buf string containined XML special characters
 * @param dest string with special characters escaped
 * @param buflen length of buf (0 means buf is nil terminated).
 * @return dest
 */
string& XMLescape(const char buf[], string& dest, Uint buflen=0);

/**
 * Replace XML escape codes with the characters they designate.
 * @param buf string containing XML escapes
 * @param dest string with escapes replaced
 * @param buflen length of buf (0 means buf is nil terminated).
 * @return dest
 */
string& XMLunescape(const char buf[], string& dest, Uint buflen=0);

}
#endif
