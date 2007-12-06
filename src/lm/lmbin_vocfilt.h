/**
 * @author Eric Joanis
 * @file lmbin_vocfilt.h  LM read from a BinLM file, with vocab filtering.
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

#ifndef __LMBIN_VOCFILT_H__
#define __LMBIN_VOCFILT_H__

#include <portage_defs.h>
#include "lmtext.h"
#include <voc_map.h>

namespace Portage {

/**
 * LM Implementation which loads a Portage BinLM language modelfile and keeps
 * it in memory, just like LMText, but with a remapped vocabulary to convert
 * from the global vocabulary to the BinLM's internal vocabulary.
 *
 * This class handles the cases where we filter the BinLM using the vocabulary
 * while loading it.  As an optimization, the vocabulary is converted while
 * loading the model, an relatively inexpensive operation, so that mapping is
 * not required when queries are made to the BinLM.  In practice, this mapping
 * can be quite expensive in setups with many different BinLM's being used, so
 * it is well worthwhile doing it at load time.
 */
class LMBinVocFilt : public LMText {

protected:
   /// Vocab map to convert between global and local vocabularies.
   VocMap voc_map;

   /**
    * Helper for read_binary(): read the trie part of the LMBin file.
    * In this class, retain only n-grams whose words are all in the global
    * vocab.
    * @param ifs stream open and located at the beginning of the binary trie
    * @param limit_order if non-zero, truncate the model to order limit_order
    * @return number of nodes kept
    */
   virtual Uint read_bin_trie(istream& ifs, Uint limit_order);

   /**
    * Read a BinLM from disk.
    * Vocab filtering is controlled by the virtual method read_bin_trie().
    * @param binlm_filename name of the BinLM file
    * @param limit_order if non-zero, truncate the model to order limit_order
    */
   void read_binary(const string& binlm_filename, Uint limit_order);

   /// Constructor for use by the subclass.
   LMBinVocFilt(Voc& vocab, OOVHandling oov_handling,
                double oov_unigram_prob);

public:
   /// Constructor.  See PLM::Create() for a description of the parameters.
   /// vocab is a Voc& instead of a Voc* because it is mandatory for this
   /// sub-class of PLM.
   /// In this class, we retain only n-grams whose words are all in vocab.
   LMBinVocFilt(const string& binlm_filename, Voc& vocab,
                OOVHandling oov_handling, double oov_unigram_prob,
                Uint limit_order);

}; // LMBinVocFilt

} // Portage

#endif // __LMBIN_VOCFILT_H__
