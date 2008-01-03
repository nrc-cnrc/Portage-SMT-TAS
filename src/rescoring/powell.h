/**
 * @author Aaron Tikuisis
 * @file powell.h  Powell's algorithm.
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

#ifndef _POWELL_H_
#define _POWELL_H_

#include <portage_defs.h>
#include <boostDef.h>
#include <linemax.h>
#include <vector>

namespace Portage
{
   using namespace std;

   template <class ScoreStats>
   class Powell {
      private:
         const vector<uMatrix>& vH;                   ///< Feature function values
         const vector< vector<ScoreStats> >& allScoreStats;  ///< translations' score
         LineMax<ScoreStats> linemax;                 ///< linemax algorithm functor

      public:
         /**
         * Default constructor.
         * @param vH      feature function values for all translations of all nbest
         * @param allScoreStats  score for all translations of all nbest. 
         *                Note that score must exists for all the life time of Powell.
         */
         Powell(const vector<uMatrix>& vH, const vector< vector<ScoreStats> >& allScoreStats)
         : vH(vH)
         , allScoreStats(allScoreStats)
         , linemax(vH, allScoreStats)
         { }

         /**
            Perform's Powell's algorithm, using Och's line maximization algorithm, in order to find parameters
            p that maximize the score on the training corpus.
            For the details of Powell's algorithm, see "Numerical Recipes in C", section 10.5.
            This algorithm attempts to find the parameter vector that will maximize the score, but it is a
            heuristic, and thus is not guarenteed to find the optimal solution.  It is recommended that
            the algorithm is run multiple times using different starting points, to improve the likelihood
            of finding the global maximum.

            We have the following setup:
            -   Training foreign sentences f_0, .. , f_{S-1}.
            -   For each s = 0, .. , S-1, we have candidate translations e_{s,0}, .. , e_{s,K-1} (eg. K-best)
            -   For each candidate translation, statistics for the score.  These statistics should be additive -
            the total score for a set of translations is determined by adding up the statistics then computing
            the score from the sum.  It is assumed that greater allScoreStats are better; if the reverse is true,
            just use the negative score instead.
            -   Feature functions h_m(e,f) for m = 0, .. , M-1

            Parameters to this function is as follows:
            p  -   A vector containing the starting point in the parameter space.  The "optimal" parameters found
            will be returned via p.
            xi -   The directions used in Powell's algorithm.  Generally, the M x M identity is a good starting
            value for this matrix.  The contents of the matrix will be modified by the algorithm.
            ftol - The fractional tolerance in the function value, such that when one iteration does not increase
            the score by at least this much, then the algorithm terminates.
            TODO: consider whether fractional tolerance is appropriate, or if some other tolerance should
            be used.
            iter - Will contain the number of iterations done in the algorithm.
            fret - Will contain the maximum score achieved.
            H  -   An array of length S, of K x M matrices containing the evaluation of the feature functions at
            the candidate translations.  Specifically, the (k, m)-th entry of H[s] should contain the
            value h_m(e_{s,k}, f_s).
            bleu - An S x K array of score statistics for the candidate sentences.  Suggestively called
            "bleu", though it is easy to write a class for other types of allScoreStats, so long as the score
            can be broken into linear components.  The following must be defined for the type ScoreStats:
            double ScoreStats::score()
            Calculates the score (bigger score is better)
            void ScoreStats::output(ostream &out)
            Output information about the stats
            ScoreStats operator+(const ScoreStats &s1, const ScoreStats &s2)
            ScoreStats operator-(const ScoreStats &s1, const ScoreStats &s2)
            Produce the sum or difference in statistics.
            By default, a new object of type ScoreStats must have "zero" statistics - that is, it should
            represent the score given to an empty set of translations.
            See bleu.h, bleu.cc (in eval) for an example of the implementation of the BLEU statistics.
            bleu[s][k] should contain the score statistics assigned to the sentence e_{s,k}.  Of course,
            the score statistics is not a function only of e_{s,k} but (presumably) of the correct
            translation as well.
            S  -   The number of source sentences.
            K  -   The number of candidate translations for each source sentence.


            * @param p     vector containing the starting point in the parameter
            *              space.
            * @param xi    directions used in Powell's algorithm.
            * @param ftol  fractional tolerance
            * @param iter  returned number of iterations done in the algorithm.
            * @param fret  returned maximum score achieved.
            */
         void operator()(uVector& p,
               uMatrix& xi,
               const double ftol,
               int &iter,
               double &fret);

         private:
            ScoreStats computeStats(const uVector& p);
            double computeScore(const uVector& p) {
               return computeStats(p).score();
            }

   };  // ends class Powell
}

#include <powell-cc.h>

#endif // _POWELL_H_
