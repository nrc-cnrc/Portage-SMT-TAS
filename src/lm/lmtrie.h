/**
 * @author Samuel Larkin
 * @file lmtrie.h  In memory representation for a language model.
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

#ifndef __LMTRIE_H__
#define __LMTRIE_H__

#include "portage_defs.h"
#include "trie.h"
#include "lm.h"

namespace Portage {

/**
 * LM implementation which loads an ARPA-formatted language model file
 * and keeps it in memory.
 */
class LMTrie : public PLM
{
public:
   /// Functor that cumulates how many xgram there are in the trie for each x=[1-N].
   struct CountVisitor {
      /// Keeps track of the N-gram counts.
      vector<Uint> counts;

      /// Default constructor.
      /// @params size  value of N
      CountVisitor(Uint size)
      : counts(size, 0)
      {}

      /**
       * Transform this class into a functor for trie.traverse.
       * @param key    phrase for this visited trie leaf.
       * @param value  value of the visited trie leaf.
       */
      void operator()(const vector<Uint> &key, float value) {
         const Uint length = key.size() - 1;
         if ( length < counts.size())
            ++counts[length];
      }
   };

protected:
   /**
    * The LM is stored internally in a trie.
    */
   PTrie<float, Wrap<float>, false> trie;

   typedef PTrie<float, Wrap<float>, false>::iterator TrieIterator;

   /// Dump the trie to cerr - for debugging purposes.
   void dump_trie_to_cerr();

   /// Recursive helper for dump_trie_to_cerr().
   void rec_dump_trie_to_cerr(vector<Uint>& key_prefix,
         PTrie<float, Wrap<float>, false>::iterator begin,
         const PTrie<float, Wrap<float>, false>::iterator& end);

   /**
    * Recursively dump the trie's leaves.
    * @param os  in which stream to dump the trie.
    * @param key_prefix what is the key prefix that lead here.
    * @param begin  start of children nodes.
    * @param end  end of children nodes.
    * @param maxDepth  maximum depth to dump.
    */
   void rec_dump_trie_arpa(
      ostream& os,
      vector<Uint>& key_prefix,
      PTrie<float, Wrap<float>, false>::iterator begin,
      const PTrie<float, Wrap<float>, false>::iterator& end,
      const Uint maxDepth
   );

public:
   struct Creator : public PLM::Creator {
      Creator(const string& lm_physical_filename, Uint naming_limit_order);
      //virtual bool checkFileExists();
      //virtual Uint64 totalMemmapSize();
      virtual PLM* Create(VocabFilter* vocab,
                          OOVHandling oov_handling,
                          float oov_unigram_prob,
                          bool limit_vocab,
                          Uint limit_order,
                          ostream *const os_filtered,
                          bool quiet);
   };

protected:
   /// Protected constructor for use by subclasses
   LMTrie(VocabFilter *vocab, OOVHandling oov_handling, double oov_unigram_prob);

   // Implemented for parent.
   virtual Uint getGramOrder() { return gram_order; }

   /**
    * Return the mapping from index in the vocab used in the trie to index in
    * the global vocab.  Trivial copy in this class, but not in subclasses.
    */
   virtual Uint global_index(Uint local_index);

   /**
    * Runs the actual trie query and back-off calculations for wordProb() -
    * intended to work for this class and for subclasses.
    * @param query  LM query, with word in query[0] and context in the rest
    * @param query_length  length of query == context_length + 1, must be > 0
    * @return the fully calculated probability result for this query
    */
   float wordProbQuery(const Uint query[], Uint query_length);

public:
   /// Destructor.
   virtual ~LMTrie();

   // implementations of virtual methods from parent class
   virtual float wordProb(Uint word, const Uint context[], Uint context_length);
   virtual float cachedWordProb(Uint word, const Uint context[], Uint context_length);
   virtual void clearCache() {}

   // Particular interface for this class

   /**
    * Write the LM out in the Portage binary language model format
    * @param binlm_file_name BinLM file name to use.
    */
   void write_binary(const string& binlm_file_name) const;

   /**
    * Write the trie/LM to NRC Portage's binary format.
    * @param os  stream where to dump the lm.
    * @param maxNgram maximum order to dump.
    */
   void write2arpalm(ostream& os, Uint maxNgram = numeric_limits<Uint>::max());

   /// Displays the memory usage and some stats of the underlying trie structure.
   void displayStats() const;

}; // ends class LMTrie

} // ends namespace Portage


#endif // __LMTRIE_H__
