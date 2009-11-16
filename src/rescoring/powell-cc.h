/**
 * @author Aaron Tikuisis
 * @file powell-cc.h  Implementation of Powell's algorithm.
 * $Id$
 *
 * K-Best Rescoring Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "rescoring_general.h"
#include "errors.h"
#include <limits>
#include <cmath>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/detail/algorithm.hpp>


using namespace Portage;
using namespace std;

namespace Portage
{


template <class ScoreStats>
void Powell<ScoreStats>::linemax(uVector& p, uVector& dir) 
{
   ++linemax_count;

   // write logging header

   if (logstream) {
      (*logstream) 
         << "linemax call " << powell_count << "." << linemax_count
         << "init feature wts (p) = " << p << endl
         << "init direction = " << dir << endl;
   }

   // do the work
   linemax_och(p, dir, logstream);

   // write logging trailer

   if (logstream) {
      const vector< pair<double,double> >& history = linemax_och.history;
      (*logstream) << "search history <index x log(BLEU(x))>:"
                   << " using " << "Och's"
                   << " linemax, " << history.size() << " iterations: " << endl;
      for (Uint i = 0; i < history.size(); ++i)
         (*logstream) << powell_count << "." << linemax_count << " "
                      << history[i].first << " " << history[i].second << endl;
      (*logstream) 
         << "linemax call " << powell_count << "." << linemax_count << " results: " << endl
         << "feature wts (p) = " << p << endl
         << "direction = " << dir << endl << endl;
   }
}

// Powell's algorithm implementation with maximization instead of minimization,
// and Och's (exact) linemax algorithm for line maximization.
template <class ScoreStats>
void Powell<ScoreStats>::operator()(uVector& p,
                                    double tolerance,
                                    int &iter,
                                    double &score)
{
   // NOTES:
   // score == f_P_N == score(P_i)

   assert(p.size() == vH[0].size2());
   score = 0.0f;
   iter  = 0;
   ++powell_count;
   linemax_count = 0;

   const Uint M(p.size());
   const double epsilon(1.0e-25);

   vector<Uint> feature_order(M);
   boost::iota(feature_order, 0);
   if (randomize_feature_order)
      random_shuffle(feature_order.begin(), feature_order.end());

   uMatrix U(M, M);
   U = uIdentityMatrix(M);

   uVector origine;
   uVector U_k;
   // Keep track of initial score before exploring all dimensions.
   // Needed outside the loop since it's part of the stopping criteria.
   double f_0(0.0f);
   score = computeScore(p);
   do {
      origine = p;       // Saving the starting point.
      f_0     = score;   // Saving the original starting point's score.

      // Keep track of the index of the largest decrease.
      double delta_f(0.0f);
      int    best_index(-1);

      for (Uint k(0); k<M; ++k) {
         U_k = uColumn(U, feature_order[k]);
         linemax(p, U_k);

         const double f_P_k     = computeScore(p);
         const double delta_f_k = fabs(f_P_k - score);
         score                  = f_P_k;
         if (delta_f_k > delta_f) {
            best_index = feature_order[k];
            delta_f    = delta_f_k;
         }
      }
      ++iter;

      if (!isfinite(score)) {
         error(ETWarn, "powell encountered infinitely small score - quitting!");
         break;
      }
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
   } while (2*fabs(f_0 - score) > tolerance*(fabs(f_0) + fabs(score)) + epsilon);
}
} // ends namespace Portage
