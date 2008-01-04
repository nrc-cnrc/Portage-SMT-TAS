/**
 * @author Nicola Ueffing
 * @file bleurisk.h  Feature Function that the risk of selecting a hypothesis w.r.t. BLEU loss
 * lists.
 * 
 * 
 * COMMENTS: 
 *
 * BLEU-based risk over nbest lists.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef BLEURISK_H
#define BLEURISK_H

#include "featurefunction.h"

namespace Portage 
{

/// Feature Function that calculates the risk of a hypothesis over the top N
/// hypotheses in the N-best list, based on BLEU as loss function.
class RiskBleu : public FeatureFunction
{
   /// Scores for one nbest list.
   vector<double> scores;
   /// number of hypothesis used as basis for risk calculation
   int lenNbest;
   /// smoothing method for sentence-level BLEU
   int smoothBleu;
   ///< Mandatory scale factor for the ff weights
   double ff_scale;
   ///< Mandatory canoe decoder features' and weights 
   string ff_wts_file;
   ///< Optional prefix for features in the wts_file
   string ff_prefix;
   
public:
   
   /// Constructor.
   /// @param args  settings for lenNbest and smoothBleu (both optional)
   RiskBleu(const string& args);
   
   virtual bool parseAndCheckArgs();

   virtual Uint requires() { return FF_NEEDS_TGT_TOKENS; }
   
   virtual FeatureFunction::FF_COMPLEXITY cost() const { return HIGH; }

   virtual void source(Uint s, const Nbest * const nbest);
   
   virtual double value(Uint k) { return scores[k]; }
};

} // ends namespace Portage

#endif 
