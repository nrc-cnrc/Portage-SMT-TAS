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
   // - The NR implementation of vectors and matrices is replaced by the GSL implementation.
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
      /////////////////////////////////////
      // external calls formerly present here:
      //  - computeScore(1 arg), defined in powell.h
      //  - linemax(2 args), a functor call to Powell::linemax, declared in
      //    powell.h and defined in linemax.h, ultimately resolved to
      //   LinaMax<Stats>::operator()(2 args).
      /////////////////////////////////////

      // NOTES:
      // score == f_P_N == score(P_i)

      assert(p.size() == vH[0].size2());
      score = 0.0f;
      iter  = 0;

      const Uint N(p.size());

      double f_0(0.0f);    // Keep track of initial score before exploring all dimensions.
      score = computeScore(p);
      do {
         uVector origine(p);  // Saving the starting point.
         f_0 = score;         // Saving the original starting point's score.
         cerr << "\tf_0: " << f_0 << endl;

         // Keep track of the index of the largest decrease.
         double delta_f(0.0f);
         int    best_index(-1);

         for (Uint k(0); k<N; ++k) {
            uVector U_k(uColumn(U, k));
            cout << "\t\tP_k: " << p << endl;
            cout << "\t\tU_k: " << U_k << U_k / ublas::norm_inf(U_k) << endl;
            linemax(p, U_k);
            cout << "\t\tP_k: " << p << endl;
            cout << "\t\tU_k: " << U_k << U_k / ublas::norm_inf(U_k) << endl;

            const double f_P_k     = computeScore(p);
            const double delta_f_k = fabs(f_P_k - score);
            score                  = f_P_k;
            cerr << "\t\tdelta_f_k: " << delta_f_k << endl;
            if (delta_f_k > delta_f) {
               best_index = k;
               delta_f    = delta_f_k;
            }
         }
         ++iter;

         cerr << "\tP: " << p << endl;
         cerr << "\tU: " << U << endl;
         cerr << "\tdelta_f: " << delta_f << endl;
         cerr << "\tbest_index: " << best_index << endl;
         //if (2*fabs(f_0 - score) <= ((fabs(f_0) + fabs(score) * tolerance))) break;

         // From this point on:
         //   p == P_N
         //   score == f_P_N == best_score == f(P_N)

         const double f_E = computeScore(2*p - origine);
         cout << "\tf_E: " << f_E << endl;
         cout << "\tf_0: " << f_0 << endl;
         // MAXIMIZATION
         if (f_E > f_0) {
            const double f_N = score;  // == f(P_N)
            const double left_hand_side = 2.0f * (-f_0 + 2.0*f_N - f_E) * square(fabs(f_0 - f_N) - delta_f);
            const double right_hand_side = delta_f * square(f_0 - f_E);
            cout << "\tf_N: " << f_N << endl;
            cout << "\tleft_hand_side: " << left_hand_side << endl;
            cout << "\tright_hand_side: " << right_hand_side << endl;
            if (left_hand_side < right_hand_side) {
               assert(best_index >= 0);  // Make sure we have found a best index.
               uVector final_dir = p - origine;

               // Replace the best/biggest decrease direction by the new average direction.
               uColumn U_r(U, best_index);
               uColumn U_N(U, N-1);
               U_r.swap(U_N);
               U_N = final_dir;// / ublas::norm_inf(final_dir);

               cout << "\t\tp: " << p << endl;
               cout << "\t\tfinal_dir: " << final_dir << final_dir / ublas::norm_inf(final_dir)  << endl;
               linemax(p, final_dir);
               cout << "\t\tp: " << p << endl;
               cout << "\t\tfinal_dir: " << final_dir << final_dir / ublas::norm_inf(final_dir)  << endl;
               score = computeScore(p);
               cerr << "\t\tU: " << U << endl;
            }
         }

         cerr << "\t" << iter << ", score: " << score << " " << p << endl;  // SAM DEBUG
         cerr << endl;
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
