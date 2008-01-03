/**
 * @author Aaron Tikuisis
 * @file linemax.h  Perform's line maximization as sketched in "Minimum Error
 * Rate Training in Statistical Machine Translation" by Franz Och.
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

#ifndef LINEMAX_H
#define LINEMAX_H

#include <portage_defs.h>
#include <boostDef.h>
#include <vector>

namespace Portage
{
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

      public:
         /**
         * Default constructor.
         * @param vH      feature function values for all translations of all nbest
         * @param allScoreStats  score for all translations of all nbest
         *                Note that score must exists for all the life time of Powell.
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
            assert(scoreWorkSpace);
            assert(gammaWorkSpace);
            for (Uint s(0); s<S; ++s) {
               delete [] gammaWorkSpace[s];
               delete [] scoreWorkSpace[s];
            }
         }

         /**
            Perform's line maximization as sketched in "Minimum Error Rate Training in Statistical Machine
            Translation" by Franz Och.
            For the details of this line maximization algorithm, see the documentation in linemax-cc.h.
            We have an initial parameter vector, p, and a direction dir along which to maximize.  This algorithm
            determines the point on the line \f$p + \gamma * dir\f$ which maximizes the score.  No heuristic is used;
            the global maximum (along the line) is found.

            We have the following setup:
            -   Training foreign sentences \f$f_0, \ldots , f_{S-1}\f$.
            -   For each \f$s = 0, .. , S-1\f$, we have candidate translations \f$e_{s,0}, \ldots , e_{s,K-1}\f$ (eg. K-best)
            -   For each candidate translation, statistics for the score.  These statistics should be additive -
            the total score for a set of translations is determined by adding up the statistics then computing
            the score from the sum.  It is assumed that greater scores are better; if the reverse is true,
            just use the negative score instead.
            -   Feature functions \f$h_m(e,f)\f$ for \f$m = 0, .. , M-1\f$

            Parameters to this function is as follows:
            p  -   A point in the parameter space, on the line along which to maximize.  When the function
            returns, contains the optimal point, p_final, along the line.
            dir -  The direction of the line along which to maximize.  When the function returns, contains the
            vector p_final - p_initial (which is a scalar multiple of the initial vector dir).
            H  -   An array of length S, of K x M matrices containing the evaluation of the feature functions at
            the candidate translations.  Specifically, the (k, m)-th entry of H[s] should contain the
            value h_m(e_{s,k}, f_s).
            allScoreStats - An S x K array of score statistics for the candidate sentences.  Suggestively called
            "allScoreStats", though it is easy to write a class for other types of scores, so long as the score
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
            allScoreStats[s][k] should contain the score statistics assigned to the sentence e_{s,k}.  Of course,
            the score statistics is not a function only of e_{s,k} but (presumably) of the correct
            translation as well.
            S  -   The number of source sentences.
            K  -   The number of candidate translations for each source sentence.


            * @param p     point in the parameter space, on the line along which to
            *              maximize.
            * @param dir   direction of the line along which to maximize.
            */
         void operator()(uVector& p, uVector& dir);

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
