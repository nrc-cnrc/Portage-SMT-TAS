/**
 * @author George Foster
 * @file casemap_strings.h  
 * 
 * COMMENTS: 
 *
 * Casemapping operations on strings, for specified locales. This handles
 * single-byte encoding schemes using the standard C++ mechanism, and utf8
 * using a custom mechanism.
 *
 * The point of this is to provide a single interface for handling normal
 * strings read in from files without having to mess with wchar_t and related
 * exotica. It also lets different conversion schemes co-exist within the same
 * program. Drawbacks are that it can't handle multi-byte coding schemes other
 * than utf8; and it depends on the external ICU library, via utf8_utils.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#ifndef CASEMAP_STRINGS_H
#define CASEMAP_STRINGS_H

#include <vector>
#include <string>
#include <locale>
#include "utf8_utils.h"

namespace Portage {

class CaseMapStrings {

   locale loc;
   UTF8Utils* utf8;

   void init();

public:

   /**
    * Create, using given locale
    */
   CaseMapStrings(const char* loc_name);
   CaseMapStrings(const locale& loc);

   /**
    * Name of the locale being used for case conversion.
    */
   string localeName() {return loc.name();}


   /**
    * Convert to uppercase. 
    * @param in  string to convert
    * @param out uppercase version of \<in\> (may be in)
    * @return out
    */
   string& toUpper(const string& in, string& out) const;

   string toUpper(const string& in) const {
      string out;
      return toUpper(in, out);
   }

   /**
    * Convert to lowercase.
    * @param in  string to convert
    * @param out lowercase version of \<out\> (may be \<in\>)
    * @return out
    */
   string& toLower(const string& in, string& out) const;

   string toLower(const string& in) const {
      string out;
      return toLower(in, out);
   }

   /**
    * Convert the first character to uppercase
    * @param in  string to convert                       
    * @param out capitalized version of \<out\> (may be \<in\>)
    * @return out
    */
   string& capitalize(const string& in, string& out) const;

   string capitalize(const string& in) const {
      string out;
      return capitalize(in, out);
   }

   /**
    * Convert the first character to lowercase.
    * @param in  string to convert                         
    * @param out decapitalized version of \<out\> (may be \<in\>)
    * @return out
    */
   string& decapitalize(const string& in, string& out) const;

   string decapitalize(const string& in) const {
      string out;
      return decapitalize(in, out);
   }

};

}

#endif /* CASEMAP_STRINGS_H */
