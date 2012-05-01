/**
 * @author George Foster
 * @file utf8_utils.h  
 * 
 * COMMENTS: 
 *
 * Portage's C++ code is generally agnostic about character coding, assuming
 * only that certain significant characters like space, newline, and vertical
 * bar retain their ASCII encoding, and can be found by scanning through byte
 * arrays. This seems to be a fairly safe strategy in general.
 * 
 * But it's not good enough for operations like case mapping, which require
 * modifying the encoded characters. I needed this for handling the new UTF8
 * encoded English text, so I put it here. This is probably not the best idea,
 * because it introduces a dependency on IBM's ICU (International Components
 * for Unicode) library right are the core of Portage. So eventually we should
 * change the implementation of this module to use some other unicode package,
 * such as the Boost one.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#ifndef UTF8_UTILS_H
#define UTF8_UTILS_H

#include <vector>
#include <string>
#ifndef NOICU
#include <unicode/ustring.h>
#endif
#include "portage_defs.h"

namespace Portage {

class UTF8Utils {

#ifndef NOICU
   vector<UChar> u16;
   vector<char> output;
   UErrorCode ecode;

   bool convToU16(const string& in); // in -> U16
   bool convFromU16(string& out); // U16 -> out
   void truncateU16AfterFirst(); // chop U16 after 1st codepoint
#endif

public:

   /**
    * Constructor.
    */
   UTF8Utils();

   /**
    * Return the status of the most recent operation.
    * @param msg If not NULL, set to a message describing the error
    * condition.
    * @return true iff ok
    */
   bool status(string* msg = NULL);

   /**
    * Convert a UTF8 string to uppercase. If the string isn't valid UTF8,
    * return \<in\> unmodified, and set error status (see status()). The
    * conversion is language independent.
    * @param in  input UTF8 string
    * @param out output UTF8 string (is allowed to be \<in\>)
    * @return out
    */
   string& toUpper(const string& in, string& out);

   /**
    * Convert a UTF8 string to lowercase. If the string isn't valid UTF8,
    * return \<in\> unmodified, and set error status (see status()). The
    * conversion is language independent.
    * @param in  input UTF8 string
    * @param out output UTF8 string (is allowed to be \<in\>)
    * @return out
    */
   string& toLower(const string& in, string& out);

   /**
    * Convert the first character of a UTF8 string to uppercase. If the string
    * isn't valid UTF8, return \<in\> unmodified, and set error status (see
    * status()). The conversion is language independent.
    * @param in  input UTF8 string
    * @param out output UTF8 string (is allowed to be \<in\>)
    * @return out
    */
   string& capitalize(const string& in, string& out);

   /**
    * Convert the first character of a UTF8 string to lowercase. If the string
    * isn't valid UTF8, return \<in\> unmodified, and set error status (see
    * status()). The conversion is language independent.
    * @param in  input UTF8 string
    * @param out output UTF8 string (is allowed to be \<in\>)
    * @return out
    */
   string& decapitalize(const string& in, string& out);

};

}

#endif /* UTF8_UTILS_H */
