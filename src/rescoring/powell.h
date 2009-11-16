/**
 * @author Aaron Tikuisis
 * @file powell.h  Powell's algorithm.
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

#ifndef _POWELL_H_
#define _POWELL_H_

#include "portage_defs.h"
#include "boostDef.h"
#include "linemax.h"
#include "eval_weights.h"
#include <vector>

namespace Portage
{
using namespace std;

/**
 * The ScoreStats class contains scoring statistics for a single translation
 * hypothesis (possibly cumulative). Usually these are BLEU statistics, though
 * it is easy to write a class for other types of ScoreStats, so long as the
 * score can be broken into linear components. The following must be defined
 * for the type ScoreStats:
 * -   double ScoreStats::score() Calculates the score (bigger score is better)
 * -   void ScoreStats::output(ostream &out) Output information about the stats
 * -   ScoreStats operator+(const ScoreStats &s1, const ScoreStats &s2)
 * -   ScoreStats operator-(const ScoreStats &s1, const ScoreStats &s2)
 * Produce the sum or difference in statistics.  By default, a new object of
 * type ScoreStats must have "zero" statistics - that is, it should represent
 * the score given to an empty set of translations.  See bleu.h, bleu.cc (in
 * eval) for an example of the implementation of the BLEU statistics.
 */
template <class ScoreStats>
class Powell
{
   private:
      const vector<uMatrix>& vH;   ///< Feature function values
      const vector< vector<ScoreStats> >& allScoreStats; ///< translations' score
      LineMax<ScoreStats> linemax_och; ///< Och's linemax algorithm functor

      /// for logging, number of calls to Powell() operator
      Uint powell_count;
      /// for logging, number of calls to linemax algorithm by Powell() operator
      Uint linemax_count;

      double computeScore(const uVector& p) {
         return evalWeights<ScoreStats>(p, vH, allScoreStats).score();
      }

      /// linemax wrapper with logging
      void linemax(uVector& p, uVector& dir);

   public:

      /// If true, choose a random order for optimizing individual features, rather
      /// than always optimizing them 1..M.
      bool randomize_feature_order;

      /// Set to a stream to have logging information written to the stream. This
      /// can be safely re-assigned or written to between calls to the Powell()
      /// operator below.  For each call to Powell(), data on every call to the
      /// linemax routine is written, along with a unique id.
      ostream* logstream;

      /**
       * Constructor. Definitions: S = number of source sentences, K = nbest
       * list size, M = number of feature functions. We have the following
       * setup:
       * -   Training foreign sentences f_0, .. , f_{S-1}.
       * -   For each s = 0, .. , S-1, K-best translations e_{s,0}, .. ,e_{s,K-1}
       * -   For each candidate translation, additive statistics for the score.
       * -   Feature functions h_m(e,f) for m = 0, .. , M-1
       * -   An optional feature transformation that modifies the global set of
       * feature values as a function of a set of "side" parameters.
       *
       * Parameters:
       *
       * @param vH An array of length S, of K x M matrices containing the
       * evaluation of the feature functions at the candidate translations.
       * Specifically, the (k, m)-th entry of H[s] should contain the value
       * h_m(e_{s,k}, f_s), where f is a source sentence, and e is a target
       * sentence. This must exist for the extent of the current object.

       * @param allScoreStats An S x K array of score statistics for the candidate
       * sentences: allScoreStats[s][k] should contain the score statistics
       * assigned to the sentence e_{s,k}. This must exist for the extent of the
       * current object. See the start of the Powell class definition for a
       * description of the ScoreStats class.

       * @param randomize_feature_order Choose a random order for optimizing
       * individual features (on each call to the Powell() operator) rather than
       * always optimizing them in 1..M order.
       */
      Powell(const vector<uMatrix>& vH,
            const vector< vector<ScoreStats> >& allScoreStats,
            bool randomize_feature_order = false)
         : vH(vH)
         , allScoreStats(allScoreStats)
         , linemax_och(vH, allScoreStats)
         , randomize_feature_order(randomize_feature_order)
         , logstream(NULL)
      {
         assert(vH.size() == allScoreStats.size());
      }

      /**
       * Perform's Powell's algorithm, using Och's line maximization
       * algorithm, in order to find parameters p that maximize the score on
       * the training corpus.
       * For the details of Powell's algorithm, see "Numerical Recipes in C",
       * section 10.5.  This algorithm attempts to find the parameter vector
       * that will maximize the score, but it is a heuristic, and thus is not
       * guaranteed to find the optimal solution.  It is recommended that the
       * algorithm be run multiple times using different starting points, to
       * improve the likelihood of finding the global maximum.
       *
       * @param p     A vector containing an initial set of feature weights.
       *              The "optimal" weights will be returned via p.
       * @param tolerance The fractional tolerance in the function value, such
       *                  that when one iteration does not increase the score by
       *                  at least this much, then the algorithm terminates.
       *                  TODO: consider whether fractional tolerance is
       *                  appropriate, or if some other tolerance should be
       *                  used.
       * @param iter  Will contain the number of iterations done in the algorithm.
       * @param score Will contain the maximum score achieved.
       */
      void operator()(uVector& p,
            double tolerance,
            int &iter,
            double &score);

};  // ends class Powell

}

#include <powell-cc.h>

#endif // _POWELL_H_
