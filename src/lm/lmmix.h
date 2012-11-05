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
   vector<double> gwts;		// .. and their (log) weights

   double* wts;                 // current weights to use

   bool sent_level_mixture;     // true if per_sent_wts are active
   vector< vector<double> > per_sent_wts; // sent index -> wts

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
    * @param notreally don't actually load component models if true
    * @param model_names write model names here if non-null
    *
    * If the file contains the line "sent-level mixture v1.0 [\<globalwt\>]" then
    * the remainer must be a sequence of weight vectors, one per line, to be
    * used in translating successive source sentences. Each per-sentence weight
    * vector must be the same size as the global vector, ie must contain a
    * weight for each component LM. If the optional \<globalwt\> parameter is
    * specified, then each per-sentence vector is set to globalwt * global-vect
    * + (1-globalwt * per-sent-vect). \<globalwt\> defaults to 0.
    */
   LMMix(const string& name, VocabFilter* vocab,
         OOVHandling oov_handling, float oov_unigram_prob,
         bool limit_vocab, Uint limit_order, bool lmmix_relative,
         bool notreally, vector<string>* model_names);

   virtual float wordProb(Uint word, const Uint context[], Uint context_length);

   virtual void newSrcSent(const vector<string>& src_sent,
                           Uint external_src_sent_id);

   const char* sentLevelMixtureCookieV1(); 
   bool sentLevelMixture() {return sent_level_mixture;}
   vector<double>& globalWeights() {return gwts;}
   vector< vector<double> >& perSentWeights() {return per_sent_wts;}

   /// Destructor.
   ~LMMix();

   virtual Hits getHits();
};

} // Portage

#endif // LMMIX_H
