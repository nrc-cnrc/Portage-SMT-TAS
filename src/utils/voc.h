/**
 * @author George Foster
 * @file voc.h Vocabulary associates an integer to every word.
 *
 *
 * COMMENTS:
 *
 * Two classes:
 *    - Voc maps strings <-> unique indexes
 *    - CountingVoc is like voc, but counts occurrences as it goes
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef VOC_H
#define VOC_H

#include "string_hash.h"
#include "file_utils.h"
#include <numeric> // for accumulate

namespace Portage {

/// Used to convert string tokens to integer value.
class Voc {
   /// When you have the index and you want to convert it back to a word.
   vector<const char*> words;
   /// When you have a word and you want to find its index.
   hash_map<const char*, Uint, hash<const char*>, str_equal> map;
   /// Iterator to traverse all the map by words
   typedef hash_map<const char*, Uint, hash<const char*>, str_equal>::const_iterator MapIter;

   /// Clears the content of the vocabulary.
   void deleteWords();

public:

   /// Functor to split string to a vector of uint using a vocab.
   struct addConverter
   {
      Voc& voc;   ///< Vocabulary used
      /// Default constructor.
      /// @param voc vocabulary to use
      addConverter(Voc& voc) : voc(voc) {}

      /**
       * Make the object a functor to map a string its uint representation.
       * @param s   source word
       * @param val uint representation of s
       * @return  Always return true
       */
      bool operator()(const char* s, Uint& val) {
         val = voc.add(s);
         return true;
      }
   };
   struct indexConverter
   {
      Voc& voc;   ///< Vocabulary used
      /// Default constructor.
      /// @param voc vocabulary to use
      indexConverter(Voc& voc) : voc(voc) {}

      /**
       * Make the object a functor to map a string its uint representation.
       * @param s   source word
       * @return Returns the uint representation of s
       */
      Uint operator()(const string& s) {
         return voc.index(s.c_str());
      }

      /**
       * Make the object a functor to map a string its uint representation.
       * @param s   source word
       * @param val uint representation of s
       * @return  Always return true
       */
      bool operator()(const char* s, Uint& val) {
         val = voc.index(s);
         return true;
      }
   };

   /// Destructor.
   ~Voc() {deleteWords();}

   /**
    * Read contents from a file in which words are listed one per line. These
    * add to current contents, if any.
    * @param filename file name to input the vocabulary
    */
   void read(const string& filename) {
      iSafeMagicStream istr(filename.c_str());
      read(istr);
   }

   /**
    * Read contents from a stream in which words are listed one per line. These
    * add to current contents, if any.
    * @param istr stream to input the vocabulary
    */
   void read(istream& istr) {
      string line;
      while (getline(istr, line)) {add(line.c_str());}
   }

   /**
    * Write contents to a file in which words are listed one per line.
    * @param filename  file name to output vocabulary
    * @param delim     delimiter between each word of the vocabulary
    */
   void write(const string& filename, const char* delim = "\n") const {
      oSafeMagicStream ostr(filename);
      write(ostr, delim);
   }

   /**
    * Write contents to a stream in which words are listed one per line.
    * @param os     stream to output vocabulary
    * @param delim  delimiter between each word of the vocabulary
    */
   void write(ostream& os, const char* delim = "\n") const {
      ostream_iterator<const char *> outStr(os, delim);
      copy(words.begin(), words.end(), outStr);
   }

   /**
    * Add a (possibly) new word.
    * @param word word to be added
    * @return index of word, equal to previous size() if new
    */
   Uint add(const char* word);

   /**
    * Get the index of a word.
    * @param word word to convert to index
    * @return index of word, or size() if not there
    */
   Uint index(const char* word) const {
      MapIter p = map.find(word);
      return p == map.end() ? size() : p->second;
   };

   /**
    * Get the index of all words.
    * This function appends the src words' uint values to dest.
    * @param src  words to convert to indexes
    * @param dest translation results from string to Uint
    */
   void index(const vector<string>& src, vector<Uint>& dest) const; 

   /**
    * Get the word associated with an index in [0,size()-1].
    * @param index index of the required word
    * @return Returns the word associated with index
    */
   const char* word(Uint index) const {return words[index];}

   /**
    * Returns the vocabulary size.
    * @return Returns the vocabulary size
    */
   Uint size() const {return words.size();}

   /// Clear the vocabulary.
   void clear() {deleteWords(); words.clear(); map.clear();}

   /**
    * Unit testing.
    * @return true if test successful
    */
   static bool test();

};

/// A vocabulary with frequency of tokens.
template<class T>
class _CountingVoc : public Voc
{
   vector<T> counts;            ///< index -> freq

public:

   /**
    * Read contents from a file in which space-separated word/count pairs are
    * listed one per line. These add to current contents, if any.
    * @param filename file name from which to get the vocabulary
    */
   void read(const string& filename) {
      iSafeMagicStream istr(filename.c_str());
      read(istr);
   }

   /**
    * Read contents from a stream in which space-separated word/count pairs are
    * listed one per line. These add to current contents, if any.
    * @param istr stream from which to get the vocabulary
    */
   void read(istream& istr) {
      string line;
      Uint n = 1;
      while (getline(istr, line)) {
         string::size_type end = line.find_last_not_of(" ");
         if (end == string::npos)
            error(ETWarn, "blank line (%d) in voc stream", n);

         string::size_type beg = line.find_last_of(" ", end);
         if (beg == string::npos)
            error(ETWarn, "badly-formed (%d) line in voc stream", n);

         string count = line.substr(beg+1, end-beg);
         end = line.find_last_not_of(" ", beg);
         if (end == string::npos)
            error(ETWarn, "badly-formed (%d) line in voc stream", n);

         string word = line.substr(0, end+1);

         if (word == "" || count == "") {
            error(ETWarn, "badly-formed (%d) line in voc stream", n);
            continue;
         }
         add(word.c_str(), conv<T>(count));
         ++n;
      }
   }

   /**
    * Add a word to the vocab, and increase its count.
    * @param word
    * @param inc amount by which to increment count
    * @return index of word
    */
   Uint add(const char* word, T inc=1) {
      Uint old_size = size();
      Uint id = Voc::add(word);
      if (id == old_size) counts.push_back(0);
      counts[id] += inc;
      return id;
   }

   /**
    * Get the frequency for an index.
    * @param index index of the desired token
    * @return the frequency of index
    */
   T& freq(Uint index) {return counts[index];}

   /**
    * Get the frequency for a word.
    * @param word token to get its frequency
    * @return the frequency of word
    */
   T freq(const char* word) {
      Uint id = index(word);
      return id == size() ? 0 : counts[id];
   }

   /**
    * Normalizes the frequencies.
    * Be careful, only works for T = floats or double
    */
   void normalize() {
      T sum = accumulate(counts.begin(), counts.end(), 0.0);
      for (Uint i = 0; i < counts.size(); ++i)
         counts[i] /= sum;
   }

};

/// Definition for the most frequently used type for the counting vocabulary.
typedef _CountingVoc<Uint> CountingVoc;

/// Unit testing of counting vocabulary.
bool testCountingVoc();

} // Portage

#endif
