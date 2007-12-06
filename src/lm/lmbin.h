/**
 * @author Eric Joanis
 * @file lmbin.h  LM read from a binlm file, without vocab filtering.
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

#ifndef __LMBIN_H__
#define __LMBIN_H__

#include <portage_defs.h>
#include "lmbin_vocfilt.h"
#include <voc_map.h>

namespace Portage {

/**
 * LM Implementation which loads a Portage BinLM language modelfile and keeps
 * it in memory, just like LMText, but with a remapped vocabulary to convert
 * from the global vocabulary to the BinLM's internal vocabulary.
 *
 * This class handles the case where the LMBin is loaded unfiltered in memory.
 * In this case, remapping the vocabulary at load time is not possible, as is
 * done in LMBinVocFilt, so the mapping is done at query time, for each query.
 */
class LMBin : public LMBinVocFilt {
   /**
    * Helper for read_binary(): read the trie part of the LMBin file.
    * In this class, no vocab filtering is applied.
    * @param ifs stream open and located at the beginning of the binary trie
    * @param limit_order if non-zero, truncate the model to order limit_order
    * @return number of nodes kept
    */
   virtual Uint read_bin_trie(istream& ifs, Uint limit_order);

protected:
   //@{
   /**
    * Overriden methods: do the same as the methods in the parent class,
    * taking the local vocabulary correctly into account.
    */
   virtual float wordProb(Uint word, const Uint context[], Uint context_length);
   virtual const char* word(Uint index) const;
   //@}

   /**
    * Return the mapping from index in the vocab used in the trie to index in
    * the global vocab.  In this class, this maps the index through the local
    * vocab.
    */
   virtual Uint global_index(Uint local_index);

public:
   /// Constructor.  See PLM::Create() for a description of the parameters.
   /// vocab is a Voc& instead of a Voc* because it is mandatory for this
   /// sub-class of PLM.
   /// In this class, no vocab filtering is applied.
   LMBin(const string& binlm_filename, Voc& vocab,
         OOVHandling oov_handling, double oov_unigram_prob,
         Uint limit_order);

}; // LMBin

} // Portage

#endif // __LMBIN_H__
