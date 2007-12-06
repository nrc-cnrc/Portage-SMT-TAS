/**
 * @author Eric Joanis
 * @file lmtext.h  In memory representation for a language model.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __LMTEXT_H__
#define __LMTEXT_H__

#include <portage_defs.h>
#include <trie.h>
#include "lm.h"

namespace Portage {

/**
 * LM implementation which loads an ARPA-formatted language model file
 * and keeps it in memory.
 */
class LMText : public PLM
{
protected:
   /**
    * The LM is stored internally in a trie.
    */
   PTrie<float, Wrap<float>, false> trie;

private:
   /**
    * Read a language model from disk
    * @param lm_file_name name of the LM file
    * @param limit_vocab if set, retain only lines whose words are all in vocab
    * @param limit_order if non-zero, truncate the model to order limit_order
    * @param os_filtered Opened stream to output the filtered LM.
    * @param quiet  Turn off verbose
    */
   void read(const string& lm_file_name, bool limit_vocab, Uint limit_order,
             ostream *const os_filtered, bool quiet);

   /**
    * Read a line from a language model file.
    * Because speed is too important when loading very large language models,
    * this method replaces PLM::readLine(), specializing it to make reading
    * large LMText files as fast as possible.
    * 
    * @param in     input stream - assumed to be at or immediately after an
    *               n-gram data line; will error(ETFatal) otherwise
    * @param prob   will be set to the prob value found on the line read
    * @param order  the order of the phrase to read, indicates which block of
    *               an ARPA LM file we are currently reading.
    * @param phrase will be set to the phrase found on the line read, in voc
    *               mapped Uints, in reversed order.  Must be of size order.
    * @param bo_wt  will be set to the back-off weight found on the line read,
    *               or 0.0 if none found
    * @param blank  will be set iff the line read is blank, in which case none
    *               of the other parameters will be modified.
    * @param bo_present  will be set iff a back-off is present on the line read
    * @param limit_vocab  if set, will skip lines with unknown words;
    *               otherwise, adds new words to the vocabulary
    * @param os_filtered Opened stream to output the filtered LM.
    */
   void readLine(istream &in, float &prob, Uint order, Uint phrase[/*order*/],
      float &bo_wt, bool &blank, bool &bo_present, bool limit_vocab,
      ostream *const os_filtered);

protected:
   /// Protected constructor for use by subclasses
   LMText(Voc *vocab, OOVHandling oov_handling, double oov_unigram_prob);

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
   // implementations of virtual methods from parent class
   virtual float wordProb(Uint word, const Uint context[], Uint context_length);
   virtual void clearCache() {}



   // Particular interface for this class

   /// Constructor.  See PLM::Create() for a description of the parameters.
   LMText(const string& lm_file_name,
          Voc *vocab,
          OOVHandling oov_handling,
          double oov_unigram_prob,
          bool limit_vocab,
          Uint limit_order,
          ostream *const os_filtered,
          bool quiet = false);

   /**
    * Write the LM out in the Portage binary language model format
    * @param binlm_file_name BinLM file name to use.
    */
   void write_binary(const string& binlm_file_name) const;

}; // class LMText

} // Portage


#endif // __LMTEXT_H__
