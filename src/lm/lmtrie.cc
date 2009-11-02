/**
 * @author Eric Joanis / Samuel Larkin
 * @file lmtrie.cc - in memory representation for a language model.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "lmtext.h"
#include "file_utils.h"
#include "lmbin_vocfilt.h"
#include "lmbin_novocfilt.h"

using namespace Portage;
using namespace std;


LMTrie::Creator::Creator(
      const string& lm_physical_filename, Uint naming_limit_order)
   : PLM::Creator(lm_physical_filename, naming_limit_order)
{}

PLM* LMTrie::Creator::Create(VocabFilter* vocab,
                            OOVHandling oov_handling,
                            float oov_unigram_prob,
                            bool limit_vocab,
                            Uint limit_order,
                            ostream *const os_filtered,
                            bool quiet)
{
   string line;
   {
      iSafeMagicStream is(lm_physical_filename, true);
      getline(is, line);
   }
   if ( line == "Portage BinLM file, format v1.0" ) {
      //cerr << "BinLM v1.0" << endl;
      assert(vocab);
      if ( limit_vocab ) {
         return new LMBinVocFilt(lm_physical_filename, *vocab, oov_handling,
               oov_unigram_prob, limit_order);
      }
      else {
         return new LMBinNoVocFilt(lm_physical_filename, *vocab, oov_handling,
               oov_unigram_prob, limit_order);
      }
   }
   else {
      //cerr << ".lmtext" << endl;
      return new LMText(lm_physical_filename, vocab, oov_handling,
            oov_unigram_prob, limit_vocab, limit_order, os_filtered, quiet);
   }
}

static const bool debug_wp = false;
static const bool debug_h = false;

void LMTrie::dump_trie_to_cerr() {
   vector<Uint> key_stack;
   cerr << "LMTrie trie dump for " << this << endl;
   rec_dump_trie_to_cerr(key_stack, trie.begin_children(), trie.end_children());
}

void LMTrie::rec_dump_trie_to_cerr(
      vector<Uint>& key_prefix,
      PTrie<float, Wrap<float>, false>::iterator begin,
      const PTrie<float, Wrap<float>, false>::iterator& end
) {
   for ( ; begin != end; ++begin ) {
      key_prefix.push_back(begin.get_key());
      if ( begin.is_leaf() ) {
         for ( vector<Uint>::const_iterator it = key_prefix.begin();
               it != key_prefix.end(); ++it )
            cerr << *it << " (" << vocab->word(*it) << ") ";
         cerr << "= " << begin.get_value();
         if ( begin.get_internal_node_value() != 0.0 )
            cerr << " " << begin.get_internal_node_value();
         cerr << endl;
      }
      if ( begin.has_children() )
         rec_dump_trie_to_cerr(key_prefix,
               begin.begin_children(), begin.end_children());
      key_prefix.pop_back();
   }
}

float LMTrie::wordProb(Uint word, const Uint context[], Uint context_length)
{
   if ( debug_wp ) cerr << "Query " << word << " | "
                        << joini(context, context + context_length) << endl;

   // Reminder: context is backwards!
   // This is desirable since we work backwards in the trie.

   TrieKeyT query[context_length+1];
   query[0] = word;
   for (Uint i = 0; i < context_length; ++i)
      query[i+1] = context[i];

   // For open-vocabulary language models, map unknown words to UNK_Symbol
   if ( complex_open_voc_lm && vocab ) {
      Uint UNK_index = vocab->index(UNK_Symbol);
      float dummy;
      for (Uint i = 0; i < context_length + 1; ++i)
         if ( ! trie.find(query+i, 1, dummy) )
            query[i] = UNK_index;
   }

   return wordProbQuery(query, context_length+1);
} // LMTrie::wordProb()

float LMTrie::wordProbQuery(const Uint query[], Uint query_length) {
   assert(query_length > 0);

   // p(w|context) is defined recursively as follows:
   // p(w|w1,..,wn) =
   //    if (exists "w1,..,wn,w")    prob("w1,..,wn,w")
   //    else if (exists "w1,..,wn") bo("w1,..,wn") + p(w|w2,..,wn)
   //    else                        p(w|w2,..,wn)
   // with bottom of the recursion defined as:
   // p(w) = if (exists "w") prob(w) else oov_unigram_prob;

   // We can simplify the recursive rule by definig bo("w1,..,wn") = 0 whenever
   // "w1,..,wn" doesn't exist.  Then we get:
   // p(w|w1,..,wn) =
   //    if (exists "w1,..,wn,w")    prob("w1,..,wn,w")
   //    else                        bo("w1,..,wn") + p(w|w2,..,wn)

   // Let i be the largest value such that "wi,..,wn,w" exists (with i==n+1
   // meaning either that only "w" exists or that even "w" doesn't exist).
   // Let prob("wi,..,w") be p("w") as defined above when i==n+1.
   // We can then unroll the recursion and obtain the following:
   //    p(w|w1,..,wn) = prob("wi,..,w") + sum j=1..i-1 (bo("wj,..,wn"))

   // The above formula is what is implemented below.

   float prob(0);
   Uint depth;
   if ( trie.find(query, query_length, prob, &depth) ) {
      if ( debug_wp ) cerr << "[" << depth << "gram prob=" << prob << "]" << endl;
      hits.hit(depth);  // Record this query's depth aka N value
      return prob;
   } else {
      hits.hit(depth);  // Record this query's depth aka N value
      // if depth==0, we need to handle oov_unigram_prob here
      if ( debug_wp ) cerr << "[" << depth << "gram prob=";
      if ( depth == 0 ) {
         prob = oov_unigram_prob;
         depth = 1;
      }

      // depth == length of longest found prefix, i.e., n+1 - (i-1) = n+2-i
      // context_length == n
      // so i = n+2-depth = context_length + 2 - depth
      // but we don't need to calculate this, since j = 1 .. i-1 is more simply
      // expressed as bo_depth = context_length down to depth

      Uint bo_min_depth = depth;
      Uint bo_max_depth = query_length - 1;
      float bo_sum_value = trie.sum_internal_node_values(
         query+1, bo_min_depth, bo_max_depth);

      if ( debug_wp ) cerr << prob << " bo_sum=" << bo_sum_value << "]" << endl;
      return
         prob +          // prob("wi,..,w")
         bo_sum_value;   // sum j=1..i-1 (bo("wj,..,wn"))
   }
} // LMTrie::wordProbQuery

float LMTrie::cachedWordProb(Uint word, const Uint context[],
                             Uint context_length)
{
   // LMTrie queries are fast enough that we don't want to cache their results.
   return wordProb(word, context, context_length);
}

LMTrie::LMTrie(VocabFilter *vocab, OOVHandling oov_handling,
               double oov_unigram_prob)
   : PLM(vocab, oov_handling, oov_unigram_prob)
   , trie(12)
{ }

LMTrie::~LMTrie()
{
}

Uint LMTrie::global_index(Uint local_index) {
   return local_index;
}

void LMTrie::write_binary(const string& binlm_file_name) const
{
   cerr << trie.getStats() << endl;

   // We use a regular output file stream because we need a seekable output
   // stream.  Can be compressed separately, after, if desired.
   ofstream ofs(binlm_file_name.c_str());
   if (!ofs)
      error(ETFatal, "unable to open %s for writing", binlm_file_name.c_str());

   // Write the "magic number" (or string...)
   ofs << "Portage BinLM file, format v1.0" << endl;

   // Write out the order of the model
   ofs << "Order = " << gram_order << endl;

   // Write out the vocabulary in plain text
   ofs << "Vocab size = " << vocab->size() << endl;
   vocab->write(ofs);
   ofs << endl;

   // Write out the trie itself - this part of the file is binary
   const Uint nodes_written = trie.write_binary(ofs);

   ofs << endl << "End of Portage BinLM file.  Internal node count="
       << nodes_written << endl;

   cerr << "Wrote out " << nodes_written << " internal nodes" << endl;

} // LMTrie::write_binary

void LMTrie::displayStats() const
{
   cerr << trie.getStats() << endl;
}


void LMTrie::rec_dump_trie_arpa(
   ostream& os,
   vector<Uint>& key_prefix,
   PTrie<float, Wrap<float>, false>::iterator begin,
   const PTrie<float, Wrap<float>, false>::iterator& end,
   const Uint maxDepth
) {
   for ( ; begin != end; ++begin ) {
      key_prefix.push_back(begin.get_key());
      if ( begin.is_leaf() && maxDepth == 0) {
         // Print probability.
         os << begin.get_value();

         // Print phrase.
         vector<Uint>::const_reverse_iterator it = key_prefix.rbegin();
         os << "\t" << word(*it);
         ++it;
         for ( ; it != key_prefix.rend(); ++it )
            os << " " << word(*it);

         // Print backoff.
         const float backoff = begin.get_internal_node_value();
         if (backoff != 0.0f)
            os << "\t" << backoff;
         os << endl;
      }

      // Process the children.
      if ( begin.has_children() )
         rec_dump_trie_arpa(os, key_prefix, begin.begin_children(), begin.end_children(), maxDepth - 1);
      key_prefix.pop_back();
   }
}

void LMTrie::write2arpalm(ostream& os, Uint maxNgram)
{
   const Uint depth = min(maxNgram, getOrder());

   // Cumulate counts for each n-gram.
   CountVisitor visitor(depth);
   trie.traverse(visitor);

   const streamsize saved_precision = os.precision();
   os.precision(7);

   os << "\n\\data\\" << "\n";
   for (Uint i(0); i<visitor.counts.size(); ++i)
      os << "ngram " << i+1 << "=" << visitor.counts[i] << "\n";
   os << endl;

   // Process each xgram separately in accordance with the arpa format.
   vector<Uint> key_prefix;
   for (Uint i(0); i<visitor.counts.size(); ++i) {
      os << "\\" << i+1 << "-grams:" << endl;
      rec_dump_trie_arpa(os, key_prefix, trie.begin_children(), trie.end_children(), i);
      os << endl;
   }
   os << "\\end\\" << endl;

   os.precision(saved_precision);
}

