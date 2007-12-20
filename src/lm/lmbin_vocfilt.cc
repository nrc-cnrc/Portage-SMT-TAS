/**
 * @author Eric Joanis
 * @file lmbin_vocfilt.cc - LMBinVocFilt implementation.
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

#include "lmbin_vocfilt.h"
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

/// Filter functor for simple vocab based filtering through the VocMap.
class VocMapBasedLMFilter {
 protected:
   /// Global/local vocab map
   VocMap& voc_map;
   /// Limit the to LM order, (non-zero if requested)
   Uint limit_order;

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
      //cerr << "Key: " << join<Uint>(key_stack) << endl;
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
   } else {
   VocMapBasedLMFilter filter(voc_map, limit_order);
   return trie.read_binary(ifs, filter, voc_map);
   }
}

void LMBinVocFilt::read_binary(const string& binlm_filename, Uint limit_order)
{
   IMagicStream ifs(binlm_filename);
   string line;
   time_t start = time(NULL);

   // "Magic string" line
   getline(ifs, line);
   if ( line != "Portage BinLM file, format v1.0" )
      error(ETFatal, "File %s not in Portage's BinLM format: bad first line",
            binlm_filename.c_str());

   // Order line
   getline(ifs, line);
   vector<string> tokens;
   split(line, tokens);
   if ( tokens.size() != 3 || tokens[0] != "Order" || tokens[1] != "=" ||
        !conv(tokens[2], gram_order ) )
      error(ETFatal, "File %s not in Portage's BinLM format: bad order line",
            binlm_filename.c_str());

   // Vocab size line
   getline(ifs, line);
   splitZ(line, tokens);
   Uint voc_size;
   if ( tokens.size() != 4 || tokens[0] != "Vocab" || tokens[1] != "size" ||
        tokens[2] != "=" || !conv(tokens[3], voc_size) )
      error(ETFatal, "File %s not in Portage's BinLM format: bad voc size line",
            binlm_filename.c_str());

   // The vocab itself
   if ( voc_map.read_local_vocab(ifs) != voc_size )
      error(ETFatal, "File %s not in Portage's BinLM format: voc size mismatch",
            binlm_filename.c_str());

   // And finally deserialize the trie, applying any caller requested filter
   Uint nodes_kept = read_bin_trie(ifs, limit_order);

   getline(ifs, line);
   if ( line != "" )
      error(ETFatal, "File %s not in Portage's BinLM format: end corrupt",
            binlm_filename.c_str());
   getline(ifs, line);
   splitZ(line,tokens,"=");
   Uint internal_nodes_in_file;
   if ( tokens.size() != 2 ||
        tokens[0] != "End of Portage BinLM file.  Internal node count" ||
        !conv(tokens[1], internal_nodes_in_file) )
      error(ETFatal, "File %s not in Portage's BinLM format: bad node count",
            binlm_filename.c_str());

   // The local to global vocab cache will not be used again, clear it now.
   voc_map.clear_caches();

   cerr << "Kept " << nodes_kept << " of " << internal_nodes_in_file
        << " nodes.  Done in " << (time(NULL) - start) << "s." << endl;
}

LMBinVocFilt::LMBinVocFilt(VocabFilter &vocab, OOVHandling oov_handling,
                           double oov_unigram_prob)
   : LMText(&vocab, oov_handling, oov_unigram_prob)
   , voc_map(vocab)
{}

LMBinVocFilt::LMBinVocFilt(const string& binlm_filename, VocabFilter &vocab,
                           OOVHandling oov_handling, double oov_unigram_prob,
                           Uint limit_order)
   : LMText(&vocab, oov_handling, oov_unigram_prob)
   , voc_map(vocab)
{
   read_binary(binlm_filename, limit_order);
   //dump_trie_to_cerr();
}
