/**
 * @author Eric Joanis
 * @file voc_map.h Maps between two vocabularies, one "local" and one "global".
 * $Id$
 * 
 * COMMENTS: 
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Conseil national de recherches du Canada / Copyright 2007, National Research Council of Canada
 */

#ifndef __VOC_MAP_H__
#define __VOC_MAP_H__

#include "portage_defs.h"
#include "voc.h"
#include <boost/dynamic_bitset.hpp>

namespace Portage {

class VocMap {

   /// Local vocabulary is private, and static once it's been read.
   Voc local_vocab;

   /// Flag indicating if we already read the local vocabulary
   bool local_vocab_read;

   /// Global vocabulary is shared with the rest of the application
   Voc& global_vocab;

   /// Map from global vocab indices to local ones.
   /// Once a value is looked up, it can never change since local_vocab is
   /// static.  All entries have the value NotLookedUp until looked up once.
   vector<Uint> global2local;

   /// NotLookedUp means the global word was not looked up in the local vocab.
   static const Uint NotLookedUp; // = (~(Uint(0)) - 1);

   /// Map from local vocab indices top local ones.
   /// Entry i is valid if
   ///     local_looked_up[i] is true and
   ///     last_global_vocab_size == global_vocab.size()
   /// or if
   ///     local2global[i] is neither NoMap nor NotLookedUp
   vector<Uint> local2global;

   /// Bit vector indicating which local2global entries are valid
   boost::dynamic_bitset<> local_looked_up;

   /// Size of the global vocabulary last time local_to_global() was called.
   Uint last_global_vocab_size;

 public:
   /// NoMap means "not in the other vocab".
   static const Uint NoMap = ~(Uint(0));

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
    * global_to_local() or local_to_global() are ever called.
    *
    * @param in input stream to read from
    * @return the size of the local vocab when done
    */
   Uint read_local_vocab(istream& in);

   /**
    * Convert from a global vocab index to a local one.
    * @param global_index global index to lookup
    * @return the matching local index if found, NoMap otherwise
    */
   Uint global_to_local(Uint global_index);

   /**
    * Convert from a local vocab index to a global one.
    * @param local_index local index to lookup
    * @return the matching global index if found, NoMap otherwise
    */
   Uint local_to_global(Uint local_index);

   /**
    * Free the memory used by the lookup caches
    */
   void clear_caches() {
      global2local.clear();
      local2global.clear();
      local_looked_up.clear();
   }

}; // VocMap

} // Portage

#endif // __VOC_MAP_H__
