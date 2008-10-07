/**
 * @author George Foster / Samuel Larkin
 * @file eval_weights.h  Evaluate weights over nbest lists
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#ifndef EVAL_WEIGHTS_H
#define EVAL_WEIGHTS_H

#include "boostDef.h"

namespace Portage
{
using namespace boost::numeric;

/**
 * Calculate the BLEU score (or other score) obtained by using a given set of
 * weights to rescore nbest hypotheses represented as feature vectors.
 * Definitions: S = number of source sentences, K = nbest list size, M = number
 * of feature functions. See powell.h for a definition of the ScoreStats class.
 * @param p vector of M weights
 * @param vH  An array of length S, of K x M matrices containing the 
 * evaluation of the feature functions at the candidate translations.
 * Specifically, the (k, m)-th entry of H[s] should contain the value
 * h_m(e_{s,k}, f_s), where f is a source sentence, and e is a target
 * sentence. This must exist for the extent of the current object.
 * @param allScoreStats  An S x K array of score statistics for the
 * candidate sentences: allScoreStats[s][k] should contain the score
 * statistics assigned to the sentence e_{s,k}. This must exist for the
 * extent of the current object.
 */
template <class ScoreStats>
ScoreStats evalWeights(const uVector& p, 
                       const vector<uMatrix>& vH, 
                       const vector< vector<ScoreStats> >& allScoreStats)
{
   // Since openmp doesn't allow to reduce struct, we do it in two phases.

   // Phase 1:
   // Get the indices of best translation for every source sentences
   const int S(vH.size());  // Number of source sentences
   assert(S>0);
   Uint best_k[S];          // Keep track of the best hypothesis for each source sentences
   int s;
#pragma omp parallel for private(s)   
   for (s=0; s<S; ++s) {
      uVector scores(vH[s].size1());
      uMatrix H_trans;

      scores = ublas::prec_prod(vH[s], p);

      best_k[s] = my_vector_max_index(scores);         // k = sentence which is scored highest
   }

   // Phase 2:
   // Equivalent to the reduce phase
   ScoreStats total;
   for (s=0; s<S; ++s) {
      const Uint k = best_k[s];
      total += allScoreStats[s][k];
   }

   return total;
}
}

#endif
