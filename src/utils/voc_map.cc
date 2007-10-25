/**
 * @author Eric Joanis
 * @file voc_map.cc Maps between two vocabularies, one "local" and one "global".
 * $Id$
 * 
 * COMMENTS: 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "voc_map.h"

using namespace Portage;

// Older compilers don't allow these definitions in the .h, so leave it here.
const Uint VocMap::NotLookedUp = (~(Uint(0)) - 1);
const Uint VocMap::NoMap = ~(Uint(0));

Uint VocMap::read_local_vocab(istream& in) {
   assert(!local_vocab_read);
   string line;
   Uint lineno(0);
   while (getline(in, line)) {
      if ( line == "" ) break;
      Uint index = local_vocab.add(line.c_str());
      if ( index + 1 != local_vocab.size() )
         error(ETWarn, "Word %s occurs twice in VocMap", line.c_str());
      ++lineno;
   }
   if ( lineno != local_vocab.size() )
      error(ETFatal, "Size of local vocab != number of lines read.");
   local_vocab_read = true;
   return lineno;
}

Uint VocMap::local_index(Uint global_index) {
   assert(local_vocab_read);
   assert(global_index < global_vocab.size());

   if ( global2local.size() <= global_index )
      global2local.resize(global_vocab.size(), NotLookedUp);

   if ( global2local[global_index] != NotLookedUp )
      return global2local[global_index];

   const char* word = global_vocab.word(global_index);
   Uint local_index = local_vocab.index(word);
   if ( local_index == local_vocab.size() ) local_index = NoMap;
   global2local[global_index] = local_index;
   return local_index;
}

Uint VocMap::global_index(Uint local_index) {
   assert(local_vocab_read);
   assert(local_index < local_vocab.size());

   // This caching may or may not actually pay off - benchmarking would be a
   // good idea.
   if ( last_global_vocab_size != global_vocab.size() ) {
      local_looked_up.reset();
      last_global_vocab_size = global_vocab.size();
   }

   if ( local2global.size() <= local_index ) {
      local2global.resize(local_vocab.size(), NotLookedUp);
      local_looked_up.resize(local_vocab.size());
   }

   if ( local2global[local_index] < NotLookedUp )
      return local2global[local_index];

   if ( local_looked_up[local_index] &&
        local2global[local_index] != NotLookedUp )
      return local2global[local_index];

   const char* word = local_vocab.word(local_index);
   Uint global_index = global_vocab.index(word);
   if ( global_index == global_vocab.size() ) global_index = NoMap;
   local2global[local_index] = global_index;
   local_looked_up.set(local_index);
   return global_index;
}

void VocMap::clear_caches() {
   global2local.clear();
   local2global.clear();
   local_looked_up.clear();
}

