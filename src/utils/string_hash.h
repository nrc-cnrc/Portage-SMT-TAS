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
 * \#include "string_hash.h"
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

#if 0
// Legacy hash_map - now replaced by tr1/unordered_map - see #else clause.
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

#define unordered_map hash_map

#else

// If these two includes cause problems at compile time, you can revert to the
// old ext/hash_map by changing the line that says #if 0 above to say #if 1.
#include <tr1/unordered_map>
#include <tr1/functional> // for tr1::hash()
//#include <backward/hash_fun.h> // for hash<const char*>
namespace std
{
   namespace tr1
   {
      /**
       * Template specialization to calculate hash values for C string, not
       * defined by default in TR1.  
       */
      template<>
         class hash<const char*>
         {
            public:
               /// Generate a hash value for a c string
               unsigned int operator()(const char* s) const {
                  // Yuk - memory allocation crazy
                  //return hash<std::string>()(s);

                  // Yuk, uses internals of implementation
                  return _Fnv_hash<>::hash(s, strlen(s));

                  // Yuk, still uses obsolete headers
                  //return __gnu_cxx::hash<const char*>()(s);
               }
         };
   } // tr1
} // std

namespace Portage {
   // Import unordered_map and hash from tr1 namespace into Portage namespace
   using std::tr1::unordered_map;
   using std::tr1::hash;

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

#endif



#endif // STRING_HASH_H
