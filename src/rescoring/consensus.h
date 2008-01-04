/**
 * @author George Foster, Nicola Ueffing
 * @file consensus.h  Feature Function that calculates a consenssus over nbest
 * lists.
 * 
 * 
 * COMMENTS: 
 *
 * Consensus over nbest lists.
 *   ConsensusWer: original variant based on WER (determining the Levenshtein alignment 
 *                 which is computationally expensive)
 *   ConsensusWin: modified variant which performs windowing over the neighbouring positions,
 *                 approximation which speeds up computation
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef CONSENSUS_H
#define CONSENSUS_H

#include "featurefunction.h"

namespace Portage 
{

/// Feature Function that calculates a consenssus over nbest lists based on
/// wer.
class ConsensusWer : public FeatureFunction
{
   /// Scores for one nbest list.
   vector<double> scores;
   
public:
   
   /// Constructor.
   /// @param dontcare  just to be compatible with all other feature function.
   ConsensusWer(const string& dontcare);
   
   virtual bool parseAndCheckArgs();

   virtual Uint requires() { return FF_NEEDS_TGT_TOKENS; }
   
   virtual FeatureFunction::FF_COMPLEXITY cost() const { return HIGH; }

   virtual void source(Uint s, const Nbest * const nbest);
   
   virtual double value(Uint k) { return scores[k]; }
};
  

/// modified variant of ConsensusWer which performs windowing over the
/// neighbouring positions.
class ConsensusWin : public FeatureFunction
{
   /// Scores for one nbest list.
   vector<double> scores;
   
public:
   
   /// Constructor.
   /// @param dontcare  just to be compatible with all other feature function.
   ConsensusWin(const string& dontcare);
   
   virtual bool parseAndCheckArgs();

   virtual Uint requires() { return FF_NEEDS_TGT_TOKENS; }

   virtual FeatureFunction::FF_COMPLEXITY cost() const { return HIGH; }

   virtual void source(Uint s, const Nbest * const nbest);
   
   virtual double value(Uint k) { return scores[k]; }
};
  
} // ends namespace Portage

#endif 
