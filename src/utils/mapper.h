/**
 * @author Samuel Larkin
 * @file mapper.h
 * @brief String mappers.
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#ifndef __MAPPER_H__
#define __MAPPER_H__

#include "word_classes.h"
#include "mm_map.h"

namespace Portage {
   struct IMapper {
      virtual ~IMapper() {}
      virtual const string& operator()(const string& in) = 0;
   };

   struct IWordClassesMapper : public IMapper {
      string unknown;   ///< What should we return if the word is not found.
      IWordClassesMapper(const string& unknown)
      : unknown(unknown)
      {}
      virtual ~IWordClassesMapper() {}
      virtual const string& operator()(const string& in) = 0;
      virtual bool empty() const = 0;
   };

   /**
    * Map words to word classes.
    */
   struct WordClassesMapper : public IWordClassesMapper, private NonCopyable {
      WordClasses word_classes; ///< Word to class mapping
      vector<string> class_str; ///< strings for class numbers

      /**
       * Default constructor.
       * @param classesFile name of the classes file mapping words to class numbers.
       * @param vocab       if non-NULL, add entries only for words also in vocab
       */
      WordClassesMapper(const string& classesFile, const string& unknown, Voc *vocab = NULL);

      virtual ~WordClassesMapper();

      /**
       * Makes this class a proper functor for VocabFilter.
       * @param in  the input word.
       * @return Returns the class for the input word as a string.
       */
      virtual const string& operator()(const string& in);

      virtual bool empty() const {
         return word_classes.size() == 0;
      }

      /**
       * Map a class number to its string representation; "<unk>" is returned
       * for unknown classes.
       * @param cls class number
       * @return Returns the string representation of a class number.
       */
      const string& getClassString(Uint cls) const {
         return cls < class_str.size() ? class_str[cls] : unknown;
      }
   };

   struct WordClassesMapper_MemoryMappedMap : public IWordClassesMapper, private NonCopyable {
      MMMap map;
      string className;

      /**
       * Default constructor.
       * @param classesFile name of the classes file mapping words to class numbers.
       */
      WordClassesMapper_MemoryMappedMap(const string& classesFile, const string& unknown);

      virtual ~WordClassesMapper_MemoryMappedMap();

      /**
       * Makes this class a proper functor for VocabFilter.
       * @param in  the input word.
       * @return Returns the class for the input word as a string.
       */
      virtual const string& operator()(const string& in);

      virtual bool empty() const {
         return map.empty();
      }
   };

   IWordClassesMapper* loadClasses(const string& fname, const string& unknown, Voc* vocab = NULL);
};

#endif  //  __MAPPER_H__
