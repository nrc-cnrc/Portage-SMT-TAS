/**
 * @author GF
 * @file lmmix.h  Dynamic LM mixture model
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef LMMIX_H
#define LMMIX_H

#include "lm.h"

namespace Portage
{

class LMMix : public PLM
{
   vector<PLM*> models;		// LMs in the mixture
   vector<double> wts;		// .. and their (log) weights

   /**
    * The gram order of the LM.
    * @return order of the lowest-order LM in the mix
    */
   Uint getGramOrder() {return gram_order;}

public:
   struct Creator : public PLM::Creator {
      Creator(const string& lm_physical_filename, Uint naming_limit_order);
      virtual bool checkFileExists();
      virtual Uint64 totalMemmapSize();
      virtual PLM* Create(VocabFilter* vocab,
                          OOVHandling oov_handling,
                          float oov_unigram_prob,
                          bool limit_vocab,
                          Uint limit_order,
                          ostream *const os_filtered,
                          bool quiet);
   };

   /**
    * Construct: name specifies the LM's in the mixture, as described below;
    * the remaining parameters are identical to those in PLM::Create().
    *
    * @param name    The name of a file containing a list of component LMs,
    *                each followed by its weight. 
    *
    * TODO: relativize the component LM names, like canoe now does with its
    * filenames.
    */
   LMMix(const string& name, VocabFilter* vocab,
         OOVHandling oov_handling, float oov_unigram_prob,
         bool limit_vocab, Uint limit_order);

   virtual float wordProb(Uint word, const Uint context[], Uint context_length);

   /// Destructor.
   ~LMMix();

   virtual Hits getHits();
};

} // Portage

#endif // LMMIX_H
