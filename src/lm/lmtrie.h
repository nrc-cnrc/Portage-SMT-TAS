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
   /**
    * Functor that cumulates the information needed to output an ngram language model in
    * ARPA format:
    *  - how many xgram there are in the trie for each x=[1-N].
    *  - which n-grams are contexts (or suffixes of a context) of a longer
    *    n-gram (to output BO weights even when they're zero).
    */
   struct ToArpaPrepVisitor {
      /// Keeps track of the N-gram counts.
      vector<Uint> counts;

      typedef PTrie<Uint, Empty, false> ContextTrie;
      /// Keeps track of all contexts of other n-grams.
      /// The key is Uint because PTrie can't use fewer than 32 bits for the key, mostly
      /// because it's optimized for that key size. So count how often it's a context, for
      /// no better reason than "we can"...
      ContextTrie contexts;

      /// Default constructor.
      /// @params size  value of N
      ToArpaPrepVisitor(Uint size)
      : counts(size, 0)
      {}

      /**
       * Transform this class into a functor for trie.traverse.
       * @param key    phrase for this visited trie leaf.
       * @param value  value of the visited trie leaf.
       */
      void operator()(const vector<Uint> &key, float value) {
         // count n-grams of each length
         const Uint length = key.size() - 1;
         if (length < counts.size())
            ++counts[length];

         // Store all suffixes of the context, which will all require an
         // explicit back-off weight, even when 0.
         for (Uint len = key.size()-1; len > 0; --len)
            contexts.at(&key[1], len) += 1;
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
    * @param visitor  Used to tell when an n-gram is a prefix and therefore
    *                 requires its BO weight even if 0
    */
   void rec_dump_trie_arpa(
      ostream& os,
      vector<Uint>& key_prefix,
      PTrie<float, Wrap<float>, false>::iterator begin,
      const PTrie<float, Wrap<float>, false>::iterator& end,
      const Uint maxDepth,
      ToArpaPrepVisitor& visitor
   );

public:
   struct Creator : public PLM::Creator {
      Creator(const string& lm_physical_filename, Uint naming_limit_order);
      //virtual bool checkFileExists(vector<string>* list);
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
   virtual Uint minContextSize(const Uint context[], Uint context_length) {
      error(ETFatal, "-minimize-lm-context-size is not supported with LMs in ARPA or binlm format, because it cannot be implemented exactly correctly in our LMTrie data structure without augmenting it. Use TPLMs instead.");
      return Uint(-1);
   }


   /**
    * Get p(word|context), but only if it exists without using back-offs.
    * @param prob will be set to the p(word|context) if found in observed data.
    * @param context            context for word, in reverse order
    * @param context_length     length of context
    * @return true iff the probability is found in the observed data.
    */
   bool rawProb(const Uint context[], Uint context_length, float& prob);

   /**
    * Get the back-off weight for context.
    * @return the back-off weight for context, or 0 if context unseen.
    */
   float wordsBW(const Uint context[], Uint length);

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

   virtual Uint getLatestNgramDepth() const { return hits.getLatestHit(); }

}; // ends class LMTrie

} // ends namespace Portage


#endif // __LMTRIE_H__
