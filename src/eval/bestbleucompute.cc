/**
 * @author Aaron Tikuisis
 * @file bestbleucompute.cc  Implementation of the heuristic that finds the best possible score for a set of source and nbest.
 *
 * $Id$
 *
 * Evaluation Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 *
 * This file contains the implementation of the best BLEU score computation, finding the
 * (probable) best BLEU score obtainable by taking one sentence from each of a set of
 * K-best lists.  It finds the optimum by iteratively improvements, one sentence at a
 * time; this is a heuristic, so it may not be the true optimum, but for reasonable data
 * it's probably correct or at least close.
 */

#include <bestbleucompute.h>
#include <portage_defs.h>
#include <errors.h>
#include <assert.h>

namespace Portage
{
   /// Tests show that convergence takes only 2 iterations.  However, iterations don't
   /// take very long so allow many more for good measure.
   static const Uint MAX_ITERATIONS = 100;

   BLEUstats Oracle::computeBestBLEU(mbIT begin,
      mbIT end,
      const Uint K,
      vector<Uint> *bestIndices,
      bool verbose,
      bool bMax)
   {
      vector<Uint> b;
      vector<Uint> &bestIndex(NULL == bestIndices ? b : *bestIndices);

      assert(end > begin);
      Uint S = (end - begin);

      assert(bestIndex.size() <= S);
      bestIndex.insert(bestIndex.end(), S - bestIndex.size(), 0);

      BLEUstats result;
      for (Uint s = 0; s < S; ++s)
      {
         assert(bestIndex[s] < K);
         result += begin[s][bestIndex[s]];
      } // for


      // Iteratively improve the best score
      Uint numIts(0);
      bool keepIterating(true);
      while (keepIterating && numIts < MAX_ITERATIONS)
      {
         if (verbose)
         {
            cout << "Iteration " << (numIts + 1) << ".  Beginning with:" << endl;
            result.output();
            cout << endl;
         } // if

         // Determine if any change is made
         keepIterating = false;
         for (Uint s = 0; s < S; ++s)
         {
            double curBest(result.score());
            Uint oldIndex(bestIndex[s]);
            result -= begin[s][bestIndex[s]];

            // Looks for a better hypothesis for that sentence
            //
            for (Uint k(0); k < K && k < begin[s].size(); ++k)
            {
               double dScore((result + begin[s][k]).score());
               // Due to a C++ error that causes dScore to be greater than curBest
               // even in the case that both are equal we added a second test
               // that verifies that the new hypothesis as different stats than the
               // current one.  If their stats are equal the new hypothesis will not
               // change the bleu score thus it is irrelevant to change the old one.
               const bool bChange(bMax ? (dScore > curBest) : (dScore < curBest));
               if (bChange && (begin[s][k] != begin[s][bestIndex[s]]))
               {
                  // Change has been made; keep iterating
                  bestIndex[s] = k;
                  curBest = dScore;
               } // if
            } // for

            result += begin[s][bestIndex[s]];
            keepIterating = keepIterating || (bestIndex[s] != oldIndex);
         } // for
         ++numIts;
      } // while

      if (keepIterating)
      {
         error(ETWarn, "Best BLEU convergence did not occur.");
      } // if

      return result;
   } // computeBestBLEU

   BLEUstats Oracle::computeBestBLEU(const MatrixBLEUstats& bleu,
     vector<Uint> *bestIndices, bool verbose, bool bMax)
   {
     assert(bleu.size() > 0);
     return Oracle::computeBestBLEU(bleu.begin(), bleu.end(), bleu[0].size(), bestIndices, verbose, bMax);
   } // computeBestBLEU
} // Portage
