/**
 * @author Eric Joanis
 * @file lmrestcost.h  Prototype implementation of better rest costs.
 *
 * Ref: Heafield, Koehn and Lavie, EMNLP-CoNLL 2012: Language Model Rest Costs
 * and Space-Efficient Storage
 * This class implements the better rest cost formula given by Heafield et al
 * 2012, as a wrapper over two LMs that encode the required information. We
 * don't (yet?) implement the optimized data structure they present, but
 * implement the logic that if the context is not fully known, using
 * lower-order probabilities out of a KN-smoothed LM is inappropriate, and use
 * estimates from separately trained lower-order models instead.
 *
 * The models should be trained using train-rest-cost-lm.sh
 * The models are invoked by naming an LM
 *    RestCost;full.{lm.gz,binlm.gz,tplm};rest-costs.{lm.gz,binlm.gz,tplm}
 * 
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2014, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2014, Her Majesty in Right of Canada
 */

#ifndef LM_RESTCOST_H
#define LM_RESTCOST_H

#include "lm.h"

namespace Portage
{

class LMRestCost : public PLM
{
   PLM* m;            ///< full-order base model
   PLM* r;            ///< rest-cost model
   Uint full_order;   ///< n-gram order of the full model, saved here for frequent use

   Uint getGramOrder() {return m->getOrder();}


public:
   /// Return true if lm_physical_filename is an LMRestCost
   static bool isA(const string& lm_physical_filename);

   struct Creator : public PLM::Creator {
      Creator(const string& lm_physical_filename, Uint naming_limit_order);
      virtual bool checkFileExists(vector<string>* list);
      virtual Uint64 totalMemmapSize();
      virtual PLM* Create(VocabFilter* vocab,
                          OOVHandling oov_handling,
                          float oov_unigram_prob,
                          bool limit_vocab,
                          Uint limit_order,
                          ostream *const os_filtered,
                          bool quiet);
   private:
      string dir;
      string configFile;
      string restCostsFile;
      string fullLMFile;
      friend class LMRestCost;
   };

private:
   const Creator myCreator; ///< keep a copy of my creator is I have all my filenames

public:
   LMRestCost(const Creator& name, VocabFilter* vocab,
              OOVHandling oov_handling, float oov_unigram_prob,
              bool limit_vocab, Uint limit_order, ostream *const os_filtered);

   /// Destructor.
   ~LMRestCost();

   virtual float wordProb(Uint word, const Uint context[], Uint context_length);
   virtual Uint minContextSize(const Uint context[], Uint context_length);
   virtual void clearCache() { m->clearCache(); r->clearCache(); }
   virtual void newSrcSent(const vector<string>& src_sent,
                           Uint external_src_sent_id);

   virtual Hits getHits() { return hits + m->getHits(); }
};

} // Portage

#endif // LM_RESTCOST_H
