/**
 * @author Aaron Tikuisis
 * @file powell-cc.h  Implementation of Powell's algorithm.
 * $Id$
 * 
 * K-Best Rescoring Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <rescoring_general.h>
#include <errors.h>
#include <limits>
#include <cmath>
#include <boost/numeric/ublas/matrix_proxy.hpp>

using namespace Portage;
using namespace std;
using namespace boost::numeric;




namespace Portage
{
   // This has been modified from the original Numerical Recipes in C++ version in 3 ways:
   // - The NR implementation of vectors and matrices is replaced by the boost implementation.
   // - The original implementation performs a minimization, while this performs a
   //   maximization.
   // - The call to a grid based line-maximization (or -minimization) method is replaced with a
   //   call to Portage::linemax, which uses Och's algorithm.
   template <class ScoreStats>
   void Powell<ScoreStats>::operator()(uVector& p,
      uMatrix& U,
      const double tolerance,
      int &iter,
      double &score)
   {
      // NOTES:
      // score == f_P_N == score(P_i)

      assert(p.size() == vH[0].size2());
      score = 0.0f;
      iter  = 0;

      const Uint M(p.size());

      double f_0(0.0f);    // Keep track of initial score before exploring all dimensions.
      score = computeScore(p);
      do {
         uVector origine(p);  // Saving the starting point.
         f_0 = score;         // Saving the original starting point's score.

         // Keep track of the index of the largest decrease.
         double delta_f(0.0f);
         int    best_index(-1);

         for (Uint k(0); k<M; ++k) {
            uVector U_k(uColumn(U, k));
            linemax(p, U_k);

            const double f_P_k     = computeScore(p);
            const double delta_f_k = fabs(f_P_k - score);
            score                  = f_P_k;
            if (delta_f_k > delta_f) {
               best_index = k;
               delta_f    = delta_f_k;
            }
         }
         ++iter;

         // From this point on:
         //   p == P_N == final point
         //   score == f_P_N == best_score == f(P_N)

         const double f_E = computeScore(2*p - origine);
         // MAXIMIZATION
         if (f_E > f_0) {
            const double f_N = score;  // == f(P_N)
            const double left_hand_side = 2.0f * (-f_0 + 2.0*f_N - f_E) * square(fabs(f_0 - f_N) - delta_f);
            const double right_hand_side = delta_f * square(f_0 - f_E);
            if (left_hand_side < right_hand_side) {
               assert(best_index >= 0);  // Make sure we have found a best index.
               uVector final_dir = p - origine;

               linemax(p, final_dir);
               score = computeScore(p);

               // Replace the best/biggest decrease direction by the new average direction.
               uColumn U_r(U, best_index);
               uColumn U_N(U, M-1);
               U_r.swap(U_N);
               U_N = final_dir;
            }
         }
      } while (2*fabs(f_0 - score) > ((fabs(f_0) + fabs(score) * tolerance)));
   } // ends Powell<ScoreStats>::operator()


   template <class ScoreStats>
   ScoreStats Powell<ScoreStats>::computeStats(const uVector& p)
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
} //  ends Powell<ScoreStats>::computeStats
} // ends namespace Portage
