/**
 * @author Eric Joanis
 * @file lmbin.cc - LMBin implementation.
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

#include "lmbin.h"
#include <file_utils.h>
#include <str_utils.h>

using namespace Portage;
using namespace std;

namespace Portage {

/// Filter functor for just filtering based on the order of the LM.
class OrderFilter {
   /// Limit the to LM order, (non-zero if requested)
   Uint limit_order;

 public:
   /// Constructor
   OrderFilter(Uint limit_order) : limit_order(limit_order) {
      assert(limit_order > 0);
   }
   /// Filter method.
   /// Return true iff key_stack should be kept.
   bool operator()(const vector<Uint>& key_stack) {
      return key_stack.size() <= limit_order;
   }
}; // OrderFilter

} // Portage

Uint LMBin::read_bin_trie(istream& ifs, Uint limit_order)
{
   if ( limit_order ) {
      OrderFilter filter(limit_order);
      return trie.read_binary(ifs, filter);
   } else {
      return trie.read_binary(ifs);
   }
}

static const Uint debug_wp = false;

float LMBin::wordProb(Uint word, const Uint context[], Uint context_length)
{
   Uint oov_index;
   if ( complex_open_voc_lm ) {
      // For complex open-vocabulary BinLM's, only words not in the local vocab
      // are oov's.  We map them to the UNK_Symbol.
      oov_index = voc_map.local_index(UNK_Symbol);
      assert(oov_index != voc_map.NoMap);
   } else {
      // otherwise, we use the largest key supported by the trie, to make sure
      // it's something not it the trie.  (voc_map.NoMap is too large, so we
      // can't just use that.)
      oov_index = trie.MaxKey;
   }

   TrieKeyT query[context_length+1];
   query[0] = voc_map.local_index(word);
   if ( query[0] == voc_map.NoMap )
      query[0] = oov_index;
   if ( debug_wp ) cerr << "Query " << word << "=>" << query[0] << " |";
   for ( Uint i(0); i < context_length; ++i ) {
      query[i+1] = voc_map.local_index(context[i]);
      if ( query[i+1] == voc_map.NoMap )
         query[i+1] = oov_index;
      if ( debug_wp ) cerr << " " << context[i] << "=>" << query[i+1];
   }
   if ( debug_wp ) cerr << endl;

   return wordProbQuery(query, context_length+1);
}

const char* LMBin::word(Uint index) const
{
   return voc_map.local_word(index);
}

Uint LMBin::global_index(Uint local_index) {
   return voc_map.global_index(local_index);
}

LMBin::LMBin(const string& binlm_filename, Voc &vocab,
             OOVHandling oov_handling, double oov_unigram_prob,
             Uint limit_order)
   : LMBinVocFilt(vocab, oov_handling, oov_unigram_prob)
{
   read_binary(binlm_filename, limit_order);
}
