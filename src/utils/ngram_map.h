/**
 * @author George Foster
 * @file ngram_map.h  Map word ngrams to arbitrary types.
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2013, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2013, Her Majesty in Right of Canada
 */

#ifndef NGRAM_MAP_H
#define NGRAM_MAP_H

#include "trie.h"
#include "voc.h"

namespace Portage {

/**
 * Compactly map word ngrams (sequences of short strings) to arbitrary types
 * (*). This is a simple convenience wrapper built on PTrie, and provides only
 * a tiny part of PTrie's dauntingly complete interface. Please extend if
 * there's a need.
 *
 * (*) Almost arbitrary: the underlying PTrie structure is instantiated with
 * NeedDtor=false, which requires that the type "can be safely freed or moved
 * without calling [its] destructors or constructors" - see trie.h for more
 * info.
 */

template <class T> class NgramMap {

   Voc voc;                     // string -> index
   PTrie<T, Empty, false> trie; // index sequence -> T

private:

   template <class Witer>
   Uint* ngram2key(Witer b, Witer e) {
      static vector<Uint> key;
      key.resize(e-b);
      for (Uint i = 0; i < key.size(); ++i, ++b)
         key[i] = voc.add(*b);
      return &key[0];
   }
   
public:

   NgramMap() {}
   ~NgramMap() { clear(); }

   /**
    * Insert an ngram->val entry into map. 
    * @param b ngram start position
    * @param e ngram end+1 position
    * @param val value to store against ngram
    * @param p_val if not null, will be set to address of inserted val (valid
    * until next operation)
    */
   template <class Witer>
   void insert(Witer b, Witer e, const T& val, T** p_val = NULL) {
      trie.insert(ngram2key(b, e), e-b, val, p_val);
   }

   /**
    * Find an ngram in map.
    * @param b ngram start position
    * @param e ngram end+1 position
    * @param will be set to address of existing val if found (valid until next
    * operation)
    * @return true iff ngram exists
    */
   template <class Witer>
   bool find(Witer b, Witer e, T*& p_val) {
      return trie.find(ngram2key(b, e), e-b, p_val);
   }

   /**
    * Find existing ngram, or insert it with default value if not found.
    * @param b ngram start position
    * @param e ngram end+1 position
    * @param will be set to address of existing/added val (valid until next
    * operation) 
    * @return true iff ngram existed already
    */
   template <class Witer>
   bool find_or_insert(Witer b, Witer e, T*& p_val) {
      return trie.find_or_insert(ngram2key(b, e), e-b, p_val);
   }

   /**
    * Remove all entries.
    */
   void clear() {voc.clear(); trie.clear();}
};


} // namespace Portage

#endif //  NGRAM_MAP_H
