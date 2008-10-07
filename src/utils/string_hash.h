/**
 * @author George Foster
 * @file string_hash.h  Definitions for hash_map and required utilities to make it
 *                      work with string and const char* in Portage.
 * 
 * COMMENTS: 
 *
 * Hash function for strings, for g++ hash_* classes; equality function for
 * const char*; name imports into the Portage namespace.
 *
 * Example:
 * 
 * #include "string_hash.h"
 * hash_map<string, MyValType> my_map
 * hash_map<const char*, MyValType, hash<const char*>, str_equal> my_map2
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef STRING_HASH_H
#define STRING_HASH_H

#include "portage_defs.h"
#include <string>
#include <cstring>
#include <ext/hash_map>

namespace __gnu_cxx
{
   /**
    * Callable entity to create hash values for string.
    * Usage: when you need a hash_map with strings as keys, simply include this
    * file and the following hash will be used by default when you leave out
    * the hash template argument: hash_map<string, MyValType>.
    */
   template<>
   class hash<std::string>
   {
    public:
      /**
       * Generates a hash value for a string
       * @param s  string to hash
       * @return Returns a hash value for s
       */
      unsigned int operator()(const std::string &s) const {
         return hash<const char *>()(s.c_str());
      } // operator()
   }; // hash<string>
} // __gnu_cxx

namespace Portage {
   // Import hash_map and hash from __gnu_cxx namespace into Portage namespace
   using __gnu_cxx::hash_map;
   using __gnu_cxx::hash;

   /**
    * Callable entity for testing equality of two const char*.
    * To use a hash on const char*, include this file only, and declare
    * hash_map<const char*, MyValType, hash<const char*>, str_equal>.
    */
   struct str_equal {
      /**
       * Compares to strings for equality.
       * @param x,y strings to compare
       * @return Returns true if x == y
       */
      bool operator()(const char* x, const char* y) const {
         return strcmp(x,y) == 0;
      }
   };
    
}


#endif // STRING_HASH_H
