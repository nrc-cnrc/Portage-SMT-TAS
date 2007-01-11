/**
 * @author Aaron Tikuisis
 * @file linemax.h  Perform's line maximization as sketched in "Minimum Error
 * Rate Training in Statistical Machine Translation" by Franz Och.
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l.information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */

#ifndef LINEMAX_H
#define LINEMAX_H

#include <boostDef.h>
#include <vector>

namespace Portage
{
    /**
    Perform's line maximization as sketched in "Minimum Error Rate Training in Statistical Machine
    Translation" by Franz Och.
    For the details of this line maximization algorithm, see the documentation in linemax.cpp.
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
     bleu - An S x K array of score statistics for the candidate sentences.  Suggestively called
      "bleu", though it is easy to write a class for other types of scores, so long as the score
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


    * @param p     point in the parameter space, on the line along which to
    *              maximize.
    * @param dir   direction of the line along which to maximize.
    * @param vH    Feature function values
    * @param bleu  score statistics for the candidate sentences.
    */

    template <class ScoreStats>
    void linemax(uVector& p,
       uVector& dir,
       const std::vector<uMatrix>& vH,
       const std::vector< std::vector<ScoreStats> >& bleu);
}

#include <linemax.cpp>

#endif // LINEMAX_H
