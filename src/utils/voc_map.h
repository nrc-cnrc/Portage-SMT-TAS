/**
 * @author Eric Joanis
 * @file voc_map.h Maps between two vocabularies, one "local" and one "global".
 * $Id$
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __VOC_MAP_H__
#define __VOC_MAP_H__

#include "portage_defs.h"
#include "voc.h"
#include <boost/dynamic_bitset.hpp>

namespace Portage {

class VocMap {

   /// Local vocabulary is private.
   /// Normally static after being read, but can be augmented using add()
   Voc local_vocab;

   /// Flag indicating if we already read the local vocabulary
   bool local_vocab_read;

   /// Global vocabulary is shared with the rest of the application
   Voc& global_vocab;

   /**
    * Map from global vocab indices to local ones.
    * All methods, including add(), maintain the following invariant:
    * Each entry has value:
    *  1) NotLookedUp: which means not valid yet, need to look it up;
    *  2) NoMap: does not exist in the local vocabulary; or
    *  3) anything else: value of existing entry in the local vocabulary
    */
   vector<Uint> global2local;

   /// NotLookedUp means the global word was not looked up in the local vocab.
   static const Uint NotLookedUp; // = (~(Uint(0)) - 1);

   /**
    * Map from local vocab indices top global ones.
    * Entry i is valid if
    *     local_looked_up[i] is true and
    *     last_global_vocab_size == global_vocab.size()
    * or if
    *     local2global[i] is neither NoMap nor NotLookedUp
    */
   vector<Uint> local2global;

   /// Bit vector indicating which local2global entries are valid
   boost::dynamic_bitset<> local_looked_up;

   /// Size of the global vocabulary last time global_index() was called.
   Uint last_global_vocab_size;

 public:
   /// NoMap means "not in the other vocab".
   static const Uint NoMap; // = ~(Uint(0));

   /**
    * Constructor.
    * @param global_vocab global vocabulary
    */
   VocMap(Voc& global_vocab)
      : local_vocab_read(false)
      , global_vocab(global_vocab)
      , last_global_vocab_size(0)
   {}

   /**
    * Read the local vocabulary from the given stream.
    *
    * The stream should contain one vocab entry per line exactly, and have its
    * end marked by a blank line.  The input should not contain duplicates.
    * The local indices will be assigned sequentially (starting at 0) to each
    * input line in their order of appearance.
    *
    * This method may only be called once, and must be called before
    * local_index() or global_index() are ever called.
    *
    * @param in input stream to read from
    * @pre add() may not have been called before
    * @return the size of the local vocab when done
    */
   Uint read_local_vocab(istream& in);

   /**
    * Add a word to both the local and the global vocabularies, and return its
    * local index.
    * Maybe not be called before read_local_vocab() has been called.
    * @param word  word to be added
    * @return local index of word, possibly newly allocated
    */
   Uint add(const char* word);

   /**
    * Get the word associated with an index in the local vocabulary
    * @param index index of the required word
    * @return The word associated with index in the local vocabulary,
    *         or "" if index >= size of local voc
    */
   const char* local_word(Uint index) const;


   /**
    * Lookup a word in the local vocabulary
    * @param word to be looked up
    * @return local index of word, or NoMap if not found.
    */
   Uint local_index(const char* word);

   /**
    * Convert from a global vocab index to a local one.
    * @param global_index global index to lookup
    * @return the matching local index if found, NoMap otherwise
    */
   Uint local_index(Uint global_index);

   /**
    * Convert from a local vocab index to a global one.
    * @param local_index local index to lookup
    * @return the matching global index if found, NoMap otherwise
    */
   Uint global_index(Uint local_index);

   /**
    * Same as global_index(Uint), so that a VocMap can be used as a functor
    * mapping local indices to global ones.
    * @param local_index local index to lookup
    * @return the matching global index if found, NoMap otherwise
    */
   Uint operator()(Uint local_index) {
      Uint result(global_index(local_index));
      assert(result != NoMap);
      return result;
   }

   /**
    * Free the memory used by the lookup caches
    */
   void clear_caches();

}; // VocMap

} // Portage

#endif // __VOC_MAP_H__
