/**
 * @author Aaron Tikuisis
 * @file powell-cc.h  Implementation of Powell's algorithm.
 * $Id$
 * 
 * K-Best Rescoring Module
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
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




////////////////////////////////////////
// COMPUTESTATS
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
      uMatrix& xi,
      const double ftol,
      int &iter,
      double &fret)
   {
      assert(vH.size() == allScoreStats.size());
      assert(p.size() == vH[0].size2());

      const double TINY(1.0e-25);

      fret = computeScore(p);
      const Uint M(p.size());
      uVector pt(p);     // Save initial point
      uVector ptt(M);
      uVector xit(M);

      for (iter=0; ; ++iter) {
         const double fp = fret;
         int ibig = 0;
         double del = 0.0;               // Will be the biggest function decrease
         for (Uint i=0; i<M; ++i) {       // In each iteration, loop over all directions in the set
            xit = ublas::column(xi, i);
            linemax(p, xit);    // Maximize along it
            const double fptt = fret;     // Make a copy of previous value
            fret = computeScore(p);
            if (fabs(fret-fptt) > del) {        // Record whether it is the largest decrease so far
               del = fabs(fret-fptt);
               ibig = i+1;
            }
         }

         if (fret == -numeric_limits<double>::infinity()) {
            error(ETWarn, "It is likely that you do not have 4-gram match thus powell will loop forever");
            break;
         }
         if (2.0*fabs(fret-fp) <= ftol*(fabs(fp)+fabs(fret))+TINY) {
            // Termination criterion
            break;
         }
         //if (iter == ITMAX) nrerror("powell exceeding maximum iterations.");
         // TODO: (maybe) put some error termination condition here.

         ptt = 2*p - pt;
         xit = p - pt;
         pt  = p;

         const double fptt = computeScore(ptt);       // Function value at extrapolated point
         if (fptt > fp) {
            const double t = 2.0*(-fp+2.0*fret-fptt)*sqr(fabs(fp-fret)-del)-del*sqr(fp-fptt);
            if (t < 0.0) {
               linemax(p, xit);   // Move to the minimum of the new direction,
               fret = computeScore(p);
               uColumn col1(xi, ibig-1);
               uColumn col2(xi, M-1);
               col1.swap(col2);
               col2 = xit;
            }
         }
      }
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
