/**
 * @author Samuel Larkin
 * @file bestbleucompute.h  Declaration of the heuristic that finds the best possible score for a set of source and nbest.
 *
 * $Id$
 *
 * Evaluation Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 *
 * This file contains the implementation of the best BLEU score computation, finding the
 * (probable) best BLEU score obtainable by taking one sentence from each of a set of
 * K-best lists.  It finds the optimum by iteratively improvements, one sentence at a
 * time; this is a heuristic, so it may not be the true optimum, but for reasonable data
 * it's probably correct or at least close.
 */

#ifndef __BEST_SCORE_COMPUTE_H__
#define __BEST_SCORE_COMPUTE_H__

#include <portage_defs.h>
#include <errors.h>

namespace Portage {

/// Tests show that convergence takes only 2 iterations.  However, iterations
/// don't take very long so allow many more for good measure.
static const Uint MAX_ITERATIONS = 100;

/**
 * Finds the best BLEU score possible produced by taking one sentence (represented by
 * the sentence's stats) from each vector in [begin, end).  The best score is found by
 * iterative improvement; thus it is a heuristic and may be incorrect.
 * @param begin The beginning of the set of vector<ScoreMetric> to use. Each
 * vector<ScoreMetric> in the range represents possible test sentences for a fixed source.
 * @param end The end of the set of vector<ScoreMetric> to use.
 * @param K  Maximum number of hypotheses to consider for each source
 * sentence. 0 means to consider all.
 * @param indexes A vector containing the index of a starting hypothesis
 * for each source sentence. If the vector is empty, or the wrong size, it will
 * be initialized with the 0th hyp for each sentence. On return, it contains
 * the index of the best hyp for each.
 * @param verbose  Whether to produce verbose output.
 * @param max true = use max bleu, false use = min bleu.
 */
template <class ScoreMetric>
ScoreMetric computeBestScore(typename vector<vector<ScoreMetric> >::const_iterator begin,
   typename vector<vector<ScoreMetric> >::const_iterator end,
   const Uint K,
   vector<Uint>& bestIndex,
   bool verbose = false,
   bool bMax = true)
{
   // initialize and compute starting score

   assert(end > begin);
   const Uint S = (end - begin);
   if (bestIndex.size() != S)
      bestIndex.assign(S, 0);
   ScoreMetric result;
   for (Uint s = 0; s < S; ++s) {
      assert(K == 0 || bestIndex[s] < K);
      result += begin[s][bestIndex[s]];
   }

   // iteratively improve the best score
   
   Uint niters = 0;
   bool keepIterating = true;
   while (keepIterating && niters < MAX_ITERATIONS) {
      if (verbose) {
         cout << "Iteration " << (niters + 1) << ".  Beginning with:" << endl;
         result.output();
         cout << endl;
      }
      // loop over source sentences, finding new best hyp for each

      keepIterating = false;
      for (Uint s = 0; s < S; ++s) {
         
         double curBest(result.score());
         Uint oldIndex(bestIndex[s]);
         result -= begin[s][bestIndex[s]];
         
         // find a new best hyp, if possible

         for (Uint k = 0; (K == 0 || k < K) && k < begin[s].size(); ++k) {

            double dScore((result + begin[s][k]).score()); // big tmp struct?

            // Due to a C++ error that causes dScore to be greater than curBest
            // even in the case that both are equal we added a second test
            // that verifies that the new hypothesis has different stats than the
            // current one.  If their stats are equal the new hypothesis will not
            // change the bleu score thus it is irrelevant to change the old one.
            const bool bChange(bMax ? (dScore > curBest) : (dScore < curBest));
            if (bChange && (begin[s][k] != begin[s][bestIndex[s]])) {
               bestIndex[s] = k;
               curBest = dScore;
            }
         }
         result += begin[s][bestIndex[s]];
         keepIterating = keepIterating || (bestIndex[s] != oldIndex);
      }
      ++niters;
   }
   
   if (keepIterating)
      error(ETWarn, "Best BLEU convergence did not occur.");
   
   return result;
}

} // ends namespace Portage

#endif // __BEST_SCORE_COMPUTE_H__
