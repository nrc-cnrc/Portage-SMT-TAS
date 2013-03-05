// $Id$
/**
 * @author Eric Joanis
 * @file tm_entry.h
 * @brief Standard class to parse a line from a phrase table, factored out
 *        so we don't need to write it in a bunch of places.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#ifndef _TM_ENTRY_H
#define _TM_ENTRY_H

#include "portage_defs.h"
#include "errors.h"
#include "str_utils.h"
#include <string>
#include <cmath>

namespace Portage {

/** 
 * Class to hold one line (and thus entry) from a phrase table in all formats
 * supported by Portage, with efficient parsing.
 */
class TMEntry : private NonCopyable {
   char* buffer;          ///< holds a copy of the original line, detructively parsed
   Uint buffer_size;      ///< current size of *buffer
   char* src;             ///< source phrase part of buffer
   char* tgt;             ///< target phrase part of buffer
   char* third;           ///< 3rd column part of buffer
   char* fourth;          ///< 4th column part of buffer
   bool reversed_table;   ///< should src and tgt, and 3rd col probs, be swapped
   string filename;       ///< filename, for error messages
   Uint lineno;           ///< Line number within filename, for error messages

   Uint third_count;      ///< Number of probs/numbers in the 3rd column
   Uint fourth_count;     ///< Number of probs/numbers in the 4th column
   bool is_initialized;   ///< Whether the first line has been read, and the counts are initialized

   void init();           ///< Do the real work for init(line)

public:

   // public constants
   static const char* sep;       ///< Standard phrase table separator: " ||| "
   static const Uint sep_len;    ///< Length of sep

   /**
    * Constructor
    * @param filename  Name of the input file, for error messages
    * @param reversed_table  If true, assume src and tgt are backwards in the
    *                        table, and that values in the 3rd column should be
    *                        swapped at the half-way point.
    */
   TMEntry(const string& filename, bool reversed_table = false);

   /// Destructor
   ~TMEntry();

   /// Parse one line from an input file to determine the format of the file
   /// @param filename  Name of the input file, for error messages
   /// @param line  First line of the file, 
   void init(const string& line);

   /// Parse a new line
   /// @param line  Next line in the input file to parse
   void newline(const string& line);

   // ===================== Destructive accessors ===================== 
   /**
    * Parse the third column into a vector of T, destroying the internal
    * buffer in the process.
    * @param elements  Array of T's of size ThirdCount() 
    * @param a_field   If non-NULL, an a= field will be allowed and, if found, stored in *a_field
    * @param c_field   If non-NULL, a c= field will be allowed and, if found, stored in *c_field
    */
   template <class T> void parseThird(T* elements,
         const char **a_field=NULL, char **c_field=NULL)
   {
      if (a_field) *a_field = NULL;
      if (c_field) *c_field = NULL;
      char* tokens[third_count+3];
      // fast, destructive split
      Uint actual_count = destructive_split(third, tokens, third_count+3);
      while (actual_count > third_count) {
         if (a_field && strncmp(tokens[actual_count-1], "a=", 2) == 0) {
            *a_field = tokens[actual_count-1] + 2;
            --actual_count;
         }
         else if (c_field && strncmp(tokens[actual_count-1], "c=", 2) == 0) {
            *c_field = tokens[actual_count-1] + 2;
            --actual_count;
         }
         else {
            // Issue an extra warning, as well as the ETFatal error that
            // will necessary get issued just after this while loop.
            error(ETWarn, "bad extra field (%s) in 3rd column in %s at line %u",
                  tokens[actual_count-1], File(), lineno);
            break;
         }
      }
      if (actual_count != third_count)
         error(ETFatal, "Wrong number of 3rd column fields (%u instead of %u) in %s at line %u",
               actual_count, third_count, File(), lineno);
      for (Uint i = 0; i < third_count; ++i) {
         if (!conv(tokens[i], elements[i]))
            error(ETFatal, "Invalid number format (%s) in %s at line %u",
                  tokens[i], File(), lineno);
         if (!isfinite(elements[i])) {
            error(ETWarn, "Invalid value of prob or score (%s) in %s at line %u",
                  tokens[i], File(), lineno);
            elements[i] = T(); // 0
         }
      }
   }

   /**
    * Parse the fourth column into a vector of T, destroying the internal
    * buffer in the process.
    * @param elements  Array of T's of size FourthCount()
    */
   template <class T> void parseFourth(T* elements)
   {
      char* tokens[fourth_count+1];
      // fast, destructive split
      const Uint actual_count = fourth ? destructive_split(fourth, tokens, fourth_count+1) : 0;
      if (actual_count != fourth_count)
         error(ETFatal, "Wrong number of 4th column fields (%u instead of %u) in %s at line %u",
               actual_count, fourth_count, File(), lineno);
      for (Uint i = 0; i < fourth_count; ++i) {
         if (!conv(tokens[i], elements[i]))
            error(ETFatal, "Invalid number format (%s) in %s at line %u",
                  tokens[i], File(), lineno);
         if (!isfinite(elements[i])) {
            error(ETWarn, "Invalid value of prob or score (%s) in %s at line %u",
                  tokens[i], File(), lineno);
            elements[i] = T(); // 0
         }
      }
   }


   // ===================== Const accessors ===================== 
   /// Get the source phrase string
   const char* Src() const { return src; }
   /// Get the target phrase string
   const char* Tgt() const { return tgt; }
   /// Get the current line number
   Uint LineNo() const { return lineno; }
   /// Get the third column count
   Uint ThirdCount() const { return third_count; }
   /// Get the fourth column count
   Uint FourthCount() const { return fourth_count; }
   /// Get the file name
   const char* File() const { return filename.c_str(); }

}; // class TMEntry

} // namespace Portage


#endif // _TM_ENTRY_H
