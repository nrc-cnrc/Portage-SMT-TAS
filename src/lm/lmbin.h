/**
 * @author Eric Joanis
 * @file lmbin.h  LMText read from a binlm file.
 * $Id$
 *
 * COMMENTS:
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __LMBIN_H__
#define __LMBIN_H__

#include <portage_defs.h>
#include "lmtext.h"
#include <voc_map.h>

namespace Portage {

/**
 * LM Implementation which loads a Portage BinLM language modelfile and keeps
 * it in memory, just like LMText, but with a remapped vocabulary to convert
 * from the global vocabulary to the BinLM's internal vocabulary.
 */
class LMBin : public LMText {
   /// Vocab map to convert between global and local vocabularies.
   VocMap voc_map;

   /**
    * Read a BinLM from disk
    * @param binlm_filename name of the BinLM file
    * @param limit_vocab if set, retain only lines whose words are all in the
    *                    global vocab
    * @param limit_order if non-zero, truncate the model to order limit_order
    */
   void read_binary(const string& binlm_filename, bool limit_vocab,
                    Uint limit_order);

protected:
   /**
    * Overriden method: does same as LMText::wordProb(), but converts the
    * word and context from the global vocab to the local vocab first.
    */
   float wordProb(Uint word, const Uint context[], Uint context_length);

public:
   /// Constructor.  See PLM::Create() for a description of the parameters.
   /// vocab is a Voc& instead of a Voc* because it is mandatory for this
   /// sub-class of PLM.
   LMBin(const string& binlm_filename, Voc &vocab, bool unk_tag,
         bool limit_vocab, Uint limit_order, double oov_unigram_prob);


}; // LMBin

} // Portage

#endif // __LMBIN_H__
