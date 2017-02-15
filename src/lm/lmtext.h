/**
 * @author Eric Joanis
 * @file lmtext.h  In memory representation for a language model.
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

#ifndef __LMTEXT_H__
#define __LMTEXT_H__

#include "portage_defs.h"
#include "lmtrie.h"

namespace Portage {

/**
 * LM implementation which loads an ARPA-formatted language model file
 * and keeps it in memory.
 */
class LMText : public LMTrie
{
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
    * @param bo_present  will be set iff a back-off is present on the line read
    * @param limit_vocab  if set, will skip lines with unknown words;
    *               otherwise, adds new words to the vocabulary
    * @param os_filtered Opened stream to output the filtered LM.
    * @return Returns true if there are more lines to process.
    */
   bool readLine(istream &in, float &prob, Uint order, Uint phrase[/*order*/],
      float &bo_wt, bool &bo_present, bool limit_vocab,
      ostream *const os_filtered);

public:
   // Particular interface for this class
   /// Verify that the file is indeed an LMText, i.e., an ARPA LM file
   static bool isA(const string& file);

   /// Constructor.  See PLM::Create() for a description of the parameters.
   LMText(const string& lm_file_name,
          VocabFilter *vocab,
          OOVHandling oov_handling,
          double oov_unigram_prob,
          bool limit_vocab,
          Uint limit_order,
          ostream *const os_filtered,
          bool quiet = false);

}; // ends class LMText

} // ends namespace Portage


#endif // __LMTEXT_H__
