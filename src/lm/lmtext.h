/**
 * @author Eric Joanis
 * @file lmtext.h  In memory representation for a language model.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Conseil national de recherches Canada / Copyright 2006, National Research Council Canada
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
   /// The LM is stored internally in a trie
   PTrie<float, Wrap<float>, false> trie;

private:
   /**
    * Read a language model from disk
    * @param lm_file_name name of the LM file
    * @param limit_vocab if set, retain only lines whose words are all in vocab
    * @param limit_order if non-zero, truncate the model to order limit_order
    * @param os_filtered Opened stream to output the filtered LM.
    */
   void read(const string& lm_file_name, bool limit_vocab, Uint limit_order, ostream *const os_filtered);

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
      float &bo_wt, bool &blank, bool &bo_present, bool limit_vocab, ostream *const os_filtered);

protected:
   // implementations of virtual methods from parent class
   float wordProb(Uint word, const Uint context[], Uint context_length);
   Uint getGramOrder() { return gram_order; }
   void clearCache() {}

   /// Protected constructor for use by subclasses
   LMText(Voc *vocab, bool unk_tag, double oov_unigram_prob);

public:
   // Particular interface for this class
   
   /// Constructor.  See PLM::Create() for a description of the parameters.
   LMText(const string& lm_file_name, Voc *vocab, bool unk_tag,
          bool limit_vocab, Uint limit_order, double oov_unigram_prob,
          ostream *const os_filtered);

   /**
    * Write the LM out in the Portage binary language model format
    * @param binlm_file_name BinLM file name to use.
    */
   void write_binary(const string& binlm_file_name) const;

}; // class LMText

} // Portage


#endif // __LMTEXT_H__
