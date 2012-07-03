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
#include <cmath>

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

   /** 
    * Use log10/exp10 everywhere, following the standard convention for LM
    * probs in Portage. Comment-out these functions to revert to old mixlm
    * behaviour, which was equivalent to raising component probs to the power
    * log10(e) before calculating the weighted combination; and also returning
    * the natural log of the combined prob instead of log10.
    */
   double log(double x) {return log10(x);}
   double log(float x) {return log10(x);}
   double exp(double x) {return pow(10, x);}
   double exp(float x) {return pow(10, x);}

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
   private:
      bool lmmix_relative;   // are paths relative to lmmix file location?
   };

   /**
    * Construct: name specifies the LM's in the mixture, as described below;
    * the remaining parameters are identical to those in PLM::Create().
    *
    * @param name    The name of a file containing a list of component LMs,
    *                each followed by its weight. 
    * @param vocab              shared vocab object for all models
    * @param oov_handling       type of vocabulary
    * @param oov_unigram_prob   the unigram prob of OOVs (if oov_handling ==
    *                           ClosedVoc)
    * @param limit_vocab   whether to restrict the LM to words already in vocab
    * @param limit_order   if non-zero, will cause the LM to be treated as
    *                      order limit_order, even if it is actually of a
    *                      higher order.
    *                      If lm_filename ends in \#N, that will also be
    *                      treated as if limit_order=N was specified.
    *                      [typical value: 0]
    * @param lmmix_relative   are paths relative to the lmmix file location?
    */
   LMMix(const string& name, VocabFilter* vocab,
         OOVHandling oov_handling, float oov_unigram_prob,
         bool limit_vocab, Uint limit_order, bool lmmix_relative);

   virtual float wordProb(Uint word, const Uint context[], Uint context_length);

   /// Destructor.
   ~LMMix();

   virtual Hits getHits();
};

} // Portage

#endif // LMMIX_H
