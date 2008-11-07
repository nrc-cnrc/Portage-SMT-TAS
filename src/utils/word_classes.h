/**
 * $Id$
 * @author Eric Joanis
 * @file word_classes.h Class to store word classes
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#ifndef WORD_CLASSES_H
#define WORD_CLASSES_H

#include "portage_defs.h"
#include "array_mem_pool.h"
#include "string_hash.h"

namespace Portage {

class WordClasses {
   /// Type of map from words to classes
   typedef unordered_map<const char*, Uint, hash<const char*>, str_equal> ClassMap;
   /// Map from words to classes
   ClassMap class_map;
   /// Storage for short strings
   ArrayMemPool<char, 128, 4096> block_storage;
   /// Highest class id seen
   Uint highest_class_id;

   /// Helper for add() and copy constructor
   void add_helper(const char* word, Uint class_id);

public:

   /**
    * Default constructor
    */
   WordClasses() : highest_class_id(0) {}

   /**
    * Copy constructor has to do a deep copy - expensive!
    */
   WordClasses(const WordClasses& wc);

   /**
    * Value meaning not found.
    */
   static const Uint NoClass;

   /**
    * Free all memory
    */
   ~WordClasses() { clear(); }

   /**
    * Empty the map and free dynamic memory
    */
   void clear();

   /**
    * @return the total number of elements in all classes.
    */
   Uint size() const;

   /**
    * @return the highest class id assigned to any word in *this
    */
   Uint getHighestClassId() const { return highest_class_id; }

   /**
    * Add word to class cl
    * @param word      the word to add
    * @param class_id  word's class
    * @return false if word is already in a different class, in which case the
    *         word is not added, true otherwise
    */
   bool add(const char* word, Uint class_id);

   /**
    * Read a file containing word classes.
    * File format: each line contains word<tab>class_id
    * The file is read from beginning to end and may not contain anything else.
    * @param class_file  file containing word classes: each line containing
    *                    word<tab>class_id
    */
   void read(const string& class_file);

   /**
    * Write the word classes in format that can be incorporated in a stream
    * and read back consistently, while still being human readable. 
    * Format: header with word count, followed by all the words in the same
    * format read() expects, and a footer.
    * @param os stream to write self to.
    */
   void writeStream(ostream& os) const;

   /**
    * Read word classes from a stream as written by writeStream().
    * Clears any preexisting contents.
    * @param is stream to read self from.
    * @param stream_name  name of stream is, or some description meaningful
    *                     to the user when an error is found in the stream.
    */
   void readStream(istream& is, const char* stream_name);

   /**
    * Lookup the class of a word.
    * @param word  word to lookup
    * @return  word's class, or NoClass if not found
    */
   Uint classOf(const char* word) const {
      ClassMap::const_iterator p = class_map.find(word);
      return p == class_map.end() ? NoClass : p->second;
   }

}; // WordClasses

} // Portage

#endif // WORD_CLASSES_H
