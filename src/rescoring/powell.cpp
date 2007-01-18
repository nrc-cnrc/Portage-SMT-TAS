/**
 * @author Aaron Tikuisis
 * @file powell.cpp  Implementation of Powell's algorithm.
 * $Id$
 * 
 * K-Best Rescoring Module
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <linemax.h>
#include <rescoring_general.h>
#include <errors.h>
#include <limits>
#include <cmath>
#include <boost/numeric/ublas/matrix_proxy.hpp>

using namespace Portage;
using namespace std;
using namespace boost::numeric;


////////////////////////////////////////
// COMPUTE STATS
template <class ScoreStats>
inline ScoreStats computeStats(uVector& p,
   const vector<uMatrix>& vH,
   const vector< vector<ScoreStats> >& bleu)
{
   ScoreStats total;
   for (Uint s(0); s<vH.size(); ++s)
   {
      uVector scores(vH[s].size1());

      scores = ublas::prec_prod(vH[s], p);

      const Uint k = my_vector_max_index(scores);         // k = sentence which is scored highest
      total = total + bleu[s][k];
   }

   return total;
}


////////////////////////////////////////
// COMPUTE SCORE
template <class ScoreStats>
inline double computeScore(uVector& p,
   const vector<uMatrix>& vH,
   const vector< vector<ScoreStats> >& bleu)
{
    return computeStats(p, vH, bleu).score();
}


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
   inline void powell(uVector& p,
      uMatrix& xi,
      const double ftol,
      int &iter,
      double &fret,
      const vector<uMatrix>& vH,
      const vector< vector<ScoreStats> >& bleu)
   {
      assert(vH.size() == bleu.size());
      assert(p.size() == vH[0].size2());

      const double TINY(1.0e-25);

      fret = computeScore(p, vH, bleu);
      const Uint M(p.size());
      uVector pt(p);     // Save initial point
      uVector ptt(M);
      uVector xit(M);

      for (iter=0; ; ++iter) {
         double fp = fret;
         int ibig = 0;
         double del = 0.0;               // Will be the biggest function decrease
         for (Uint i=0; i<M; ++i) {       // In each iteration, loop over all directions in the set
            xit = ublas::column(xi, i);
            double fptt = fret;
            linemax(p, xit, vH, bleu);    // Maximize along it
            fret = computeScore(p, vH, bleu);
            if (fabs(fret-fptt) > del) {        // Record whether it is the largest decrease so far
               del = fabs(fret-fptt);
               ibig = i+1;
            }
         }

         if (fret == -numeric_limits<double>::infinity())
         {
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

         double fptt = computeScore(ptt, vH, bleu);       // Function value at extrapolated point
         if (fptt > fp) {
            double t = 2.0*(-fp+2.0*fret-fptt)*SQR(fabs(fp-fret)-del)-del*SQR(fp-fptt);
            if (t < 0.0) {
               linemax(p, xit, vH, bleu);   // Move to the minimum of the new direction,
               fret = computeScore(p, vH, bleu);
               uColumn col1(xi, ibig-1);
               uColumn col2(xi, M-1);
               col1.swap(col2);
               col2 = xit;
            }
         }
      }
   }
}
