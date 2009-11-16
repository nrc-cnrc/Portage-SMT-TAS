/**
 * @author Aaron Tikuisis
 * @file linemax.h  Performs line maximization as sketched in "Minimum Error
 * Rate Training in Statistical Machine Translation" by Franz Och.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef LINEMAX_H
#define LINEMAX_H

#include "portage_defs.h"
#include "boostDef.h"
#include <vector>
#include <boost/checked_delete.hpp>
#include <algorithm>

namespace Portage
{

// See powell.h for description of ScoreStats class

template <class ScoreStats>
class LineMax
{
public:
   /// A structure used to allow gamma values to be sorted, while maintaining
   /// reference to the associated source sentence (s) and index (i).
   struct linemaxpt
   {
      double gamma;   ///< Gamma value
      Uint s;         ///< index of source sentence.
      Uint i;         ///< index.

      /**
       * Sorts in decreasing order p1 and p2
       * @param p1  left-hand side operand
       * @param p2  right-hand side operand
       * @return Returns p1 > p2
       */
      static bool greater(const linemaxpt * const p1, const linemaxpt * const p2) {
         return p1->gamma > p2->gamma;
      }

      /**
       * Checks if the linemaxpt pointer is NULL
       * @param  p  a pointer to a linemaxpt
       * @return Returns true if the point is NULL
       */
      static bool isNull(const linemaxpt* const p) {
         return p == NULL;
      }
   };

private:
   const Uint S;                                ///< number of source sentences
   const vector<uMatrix>& vH;                   ///< Feature function values
   const vector< vector<ScoreStats> >& allScoreStats;  ///< translations' score
   ScoreStats** scoreWorkSpace;                 ///< efficient ScoreStats workspace
   double** gammaWorkSpace;                     ///< efficient gamma workspae
   bool record_history;

public:
   /**
    * Constructor. Definitions: S = number of source sentences, K = nbest list
    * size, M = number of feature functions.
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
   LineMax(const vector<uMatrix>& vH, const vector< vector<ScoreStats> >& allScoreStats)
      : S(allScoreStats.size())
      , vH(vH)
      , allScoreStats(allScoreStats)
   {
      assert(vH.size() == allScoreStats.size());
      scoreWorkSpace = new ScoreStats*[S];
      assert(scoreWorkSpace);
      gammaWorkSpace = new double*[S];
      assert(gammaWorkSpace);
      for (Uint s(0); s<S; ++s) {
         scoreWorkSpace[s] = new ScoreStats[allScoreStats[s].size()];
         gammaWorkSpace[s] = new double[allScoreStats[s].size()];
      }
   }
   /// Destructor.
   ~LineMax() {
      using namespace boost;
      using namespace std;

      assert(scoreWorkSpace);
      for_each(scoreWorkSpace, scoreWorkSpace+S, checked_array_delete<ScoreStats>);
      checked_array_delete(scoreWorkSpace);

      assert(gammaWorkSpace);
      for_each(gammaWorkSpace, gammaWorkSpace+S, checked_array_delete<double>);
      checked_array_delete(gammaWorkSpace);
   }

   /// Set of (gamma,score(gamma)) pairs
   vector< pair<double,double> > history;

   /**
      Performs line maximization as sketched in "Minimum Error Rate Training in
      Statistical Machine Translation" by Franz Och.  For the details of this
      line maximization algorithm, see the documentation in linemax-cc.h.  We
      have an initial parameter vector, p, and a direction dir along which to
      maximize.  This algorithm determines the point on the line \f$p + \gamma
      * dir\f$ which maximizes the score.  No heuristic is used; the global
      maximum (along the line) is found. We have the following setup:
       -   Training foreign sentences f_0, .. , f_{S-1}.
       -   For each s = 0, .. , S-1, K-best translations e_{s,0}, .. ,e_{s,K-1}
       -   For each candidate translation, additive statistics for the score.
       -   Feature functions h_m(e,f) for m = 0, .. , M-1
       -   An optional feature transformation that modifies the global set of
       feature values as a function of a set of "side" parameters (these
       parameters are not optimized by this function).

       Parameters to this function are as follows:

       @param p A point in the parameter space (ie, a set of feature weights),
       on the line along which to maximize.  When the function returns,
       contains the optimal point, p_final, along the line.

       @param dir The direction of the line along which to maximize.  When the
       function returns, contains the vector p_final - p_initial (which is a
       scalar multiple of the initial vector dir).

       @param record_history Store all function evaluations in "history" vector
       if true.
   */
   void operator()(uVector& p, uVector& dir, bool record_history=false);


private:
   /**
    * @param numchanges
    * @param gamma
    * @param dBLEU
    * @param curBLEU
    * @param myHeappoint
    * @param s
    * @param p
    * @param dir
    * @param H
    * @param bleu
    */
   void findSentenceIntervals(Uint &numchanges,
                              double *& gamma,
                              ScoreStats *&dBLEU,
                              ScoreStats &curBLEU,
                              linemaxpt*& myHeappoint,
                              const int s,
                              const uVector& p,
                              const uVector& dir,
                              const uMatrix& H,
                              const vector<ScoreStats>& bleu);
}; // ends class LineMax
} // ends namespace Portage

#include <linemax-cc.h>

#endif // LINEMAX_H
