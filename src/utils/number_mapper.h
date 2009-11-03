/**
 * @author Samuel Larkin
 * @file number_mapper.h - Replaces digits of a number into a token (@)
 * 
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
#ifndef __MAP_NUMBER_H__
#define __MAP_NUMBER_H__

#include "portage_defs.h"
#include <string>

namespace Portage {
namespace NumberMapper {

using namespace std;

class baseNumberMapper;
baseNumberMapper* getMapper(const string& map_type, const char token = '@');

/// Base class for digit mapper.
/// example: 186,000.00 => @@@,@@@.@@
class baseNumberMapper {
   protected:
      const char token;     ///< token to replace digit
      string     workspace; ///< a convenient temporary workplace

   public:
      /**
      * Functor to make any number mapper pointer a compatible converter for split.
      */
      struct functor {
         /// What number mapper this capsule is wrapping.
         baseNumberMapper*  mapper;

         /**
         * Default constructor.
         * @param map_type  the number mapper type.
         * @param token     token to replace digits.
         */
         functor(const string& map_type, const char token = '@')
         : mapper(getMapper(map_type, token))
         {
            assert(mapper != NULL);
         }
         /**
         * Default constructor.
         * @param mapper  the number mapper.
         */
         functor(baseNumberMapper* const mapper)
         : mapper(mapper)
         {
            assert(mapper != NULL);
         }
         /**
         * Free the memory used by the internal mapper.
         * Since the capsule cannot really be the owner of the pointer, because
         * split need copy to copy the converter aka the capsule, we still offer the
         * possibility for the user to release the memory of the mapper.
         */
         void free() {
            if (mapper) {
               delete mapper;
               mapper = NULL;
            }
         }
         /**
         * Makes the capsule a proper functor for split.
         * @param in   the input string to map.
         * @param out  the mapped string.
         */
         void operator()(const string& in, string& out) {
            assert(mapper != NULL);
            mapper->map(in, out);
         }
      };

   public:
      /// Default constructor.
      /// @param token  token to replace digit [@]
      baseNumberMapper(const char token = '@');
      /// Default destructor.
      virtual ~baseNumberMapper();

      /**
       * Substitutes digit by a token.
       * @param in  input word.
       * @param out mapped input result.
       */
      virtual void map(const string& in, string& out);

      /**
       * Converter functor to be used by split.
       * @param in   the input string
       * @param out  the mapped string
       */
      void operator()(const string& in, string& out);

}; // ends class mapNumber


/**
 * Substitutes digit by token in words that only contains digits and
 * punctuations.
 * example: 186,000.00 => @@@,@@@.@@
 *          45C => 45C
 *          hello => hello
 */ 
class simpleNumberMapper : public baseNumberMapper {
   public:
      /// Default constructor.
      /// @param token  token to replace digit [@]
      simpleNumberMapper(const char token = '@');

      virtual void map(const string& in, string& out);

}; // ends class mapNumber


/**
 * Substitutes digits by token which are prefix of a word.
 * example: 186,000.00 => @@@,@@@.@@
 *          45C => @@C
 *          hello => hello
 */
class prefixNumberMapper : public baseNumberMapper {
   public:
      /// Default constructor.
      /// @param token  token to replace digit [@]
      prefixNumberMapper(const char token = '@');

      virtual void map(const string& in, string& out);

}; // ends class mapNumber
} // ends namespace NumberMapper
} // ends namespace Portage

#endif // __MAP_NUMBER_H__
