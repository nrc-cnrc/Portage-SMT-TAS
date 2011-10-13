/**
 * @author Eric Joanis
 * @file lmbin_vocfilt.cc - LMBinVocFilt implementation.
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

#include "lmbin_vocfilt.h"
#include "file_utils.h"
#include "str_utils.h"
#include <boost/dynamic_bitset.hpp>

using namespace Portage;
using namespace std;

namespace Portage {

/// Filter functor for simple vocab based filtering through the VocMap.
class VocMapBasedLMFilter {
 protected:
   /// Global/local vocab map
   VocMap& voc_map;
   /// Limit the to LM order, (non-zero if requested)
   const Uint limit_order;

 public:
   /// Destructor
   virtual ~VocMapBasedLMFilter() {}
   /// Constructor
   VocMapBasedLMFilter(VocMap& voc_map, Uint limit_order)
      : voc_map(voc_map), limit_order(limit_order) {}
   /// Filter method.
   /// Return true iff key_stack should be kept.
   /// Looks only at the last key, since the other ones were already checked.
   virtual bool operator()(const vector<Uint>& key_stack) {
      return
         (limit_order == 0 || key_stack.size() <= limit_order) &&
         voc_map.global_index(key_stack.back()) != voc_map.NoMap;
   }
}; // VocMapBasedLMFilter

/// Filter functor for Ghada's phrase I LM filtering: per sentence vocabulary
class PerSentVocLMFilter : private VocMapBasedLMFilter {
   /// Per sentence vocabulary
   MultiVoc& per_sentence_vocab;

 public:
   /// Constructor
   PerSentVocLMFilter(MultiVoc& per_sentence_vocab, VocMap& voc_map,
                      Uint limit_order)
      : VocMapBasedLMFilter(voc_map, limit_order)
      , per_sentence_vocab(per_sentence_vocab) {}

   /// Filter method.
   /// Return true iff key_stack should be kept.
   /// If the base class filter passes, do the per-sentence check.
   bool operator()(const vector<Uint>& key_stack) {
      //cerr << "Key: " << join(key_stack) << endl;
      if ( ! VocMapBasedLMFilter::operator()(key_stack) ) return false;
      if ( key_stack.size() == 1 ) return true;
      boost::dynamic_bitset<>
         intersection(per_sentence_vocab.get_vocs(
                                       voc_map.global_index(key_stack[0])));
      //cerr << "0: " << intersection << endl;
      for ( Uint i = 1; i < key_stack.size(); ++i ) {
         intersection &= per_sentence_vocab.get_vocs(
                                       voc_map.global_index(key_stack[i]));
         //cerr << i << ": " << per_sentence_vocab.get_vocs(
         //                     voc_map.global_index(key_stack[i])) << endl;
      }
      //cerr << "I: " << intersection << endl;
      return intersection.any();
   }
}; // PerSentVocLMFilter

} // Portage

Uint LMBinVocFilt::read_bin_trie(istream& ifs, Uint limit_order)
{
   if ( vocab->per_sentence_vocab ) {
      PerSentVocLMFilter filter(*(vocab->per_sentence_vocab), voc_map,
                                limit_order);
      return trie.read_binary(ifs, filter, voc_map);
   }
   else {
      VocMapBasedLMFilter filter(voc_map, limit_order);
      return trie.read_binary(ifs, filter, voc_map);
   }
}

LMBinVocFilt::LMBinVocFilt(const string& binlm_filename,
      VocabFilter &vocab,
      OOVHandling oov_handling, 
      double oov_unigram_prob,
      Uint limit_order)
   : LMBin(&vocab, oov_handling, oov_unigram_prob)
{
   read_binary(binlm_filename, limit_order);
   //dump_trie_to_cerr();
}
