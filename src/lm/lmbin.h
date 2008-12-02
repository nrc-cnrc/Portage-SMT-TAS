/**
 * @author Eric Joanis
 * @file lmbin.h  LM read from a binlm file, without vocab filtering.
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

#ifndef __LMBIN_H__
#define __LMBIN_H__

#include "portage_defs.h"
#include "lmtrie.h"
#include "voc_map.h"

namespace Portage {

/**
 * LM Implementation which loads an LM language modelfile in Portage's BinLM
 * format in memory.
 */
class LMBin : public LMTrie {
   /**
    * Helper for read_binary(): read the trie part of the LMBin file.
    * In this class, no vocab filtering is applied.
    * @param ifs stream open and located at the beginning of the binary trie
    * @param limit_order if non-zero, truncate the model to order limit_order
    * @return number of nodes kept
    */
   virtual Uint read_bin_trie(istream& ifs, Uint limit_order) = 0;

protected:
   /// Vocab map to convert between global and local vocabularies.
   VocMap voc_map;

   /**
    * Read a BinLM from disk.
    * Vocab filtering is controlled by the virtual method read_bin_trie().
    * @param binlm_filename name of the BinLM file
    * @param limit_order if non-zero, truncate the model to order limit_order
    */
   void read_binary(const string& binlm_filename, Uint limit_order);

   /// Protected constructor for use by subclasses
   LMBin(VocabFilter *vocab, OOVHandling oov_handling, double oov_unigram_prob);

   /// Destructor.
   virtual ~LMBin();

}; // ends class LMBin

} // ends namespace Portage

#endif // __LMBIN_H__
