#ifndef __WORD_CLASS_MAPPER__
#define __WORD_CLASS_MAPPER__

#include "portage_defs.h"
#include "printCopyright.h"
#include "mm_map.h"
#include <tr1/unordered_map>


namespace Portage {

/**
 * Interface to map words to their classes in NNJM's context.
 */
struct IWordClassesMapper {
   const string unknown;   ///< What will be returned if the word is not part of the map.

   /**
    * Default constructor.
    * @param  unknown  What will be returned if the word is not part of the map.
    */
   IWordClassesMapper(const string& unknown)
   : unknown(unknown)
   {}

   /// Destructor.
   virtual ~IWordClassesMapper() {}

   /**
    * Maps a word to its class.
    * @params  word  word for which we want its class.
    * @return  word's class or unknown.
    */
   virtual const string& operator()(const string& word) const = 0;

   /**
    * Is the map empty?
    * @return  true if their is no element in the map.
    */
   virtual bool empty() const = 0;

   static bool isMemoryMapped(const string& fname);
};



struct WordClassesTextMapper : public IWordClassesMapper, private NonCopyable {
   typedef std::tr1::unordered_map<string, string> Word2class;   ///< word -> class.
   Word2class word2class;   ///< word -> class.

   /**
    * Default constructor.
    * @param  fname  text file name containing word -> class.
    * @param  unknown  What will be returned if the word is not part of the map.
    */
   WordClassesTextMapper(const string& fname, const string& unknown);

   /**
    * Maps a word to its class.
    * @params  word  word for which we want its class.
    * @return  word's class or unknown.
    */
   virtual const string& operator()(const string& word) const;

   /**
    * Is the map empty?
    * @return  true if their is no element in the map.
    */
   virtual bool empty() const { return word2class.empty(); }
};



struct WordClassesMemoryMappedMapper : public IWordClassesMapper, private NonCopyable {
   MMMap word2class;   ///<  Memory Mapped map  word -> class.
   mutable string className;   ///< Placeholder for word's class name.

   /**
    * Default constructor.
    * @param  fname  text file name containing word -> class.
    * @param  unknown  What will be returned if the word is not part of the map.
    */
   WordClassesMemoryMappedMapper(const string& fname, const string& unknown);

   /**
    * Maps a word to its class.
    * @params  word  word for which we want its class.
    * @return  word's class or unknown.
    */
   virtual const string& operator()(const string& word) const;

   /**
    * Is the map empty?
    * @return  true if their is no element in the map.
    */
   virtual bool empty() const { return word2class.empty(); }
};



/**
 * Factory that either loads a text or memory mapped map word to classes file.
 * @param fname  word classes filename either text or memory mapped map.
 * @param unknown  What will be returned if the word is not part of the map.
 */
IWordClassesMapper* getWord2ClassesMapper(const string& fname, const string& unknown);


};   // Ends namespace Portage

#endif   // __WORD_CLASS_MAPPER__
