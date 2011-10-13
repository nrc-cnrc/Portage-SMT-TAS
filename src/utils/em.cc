/**
 * @author George Foster
 * @file em.cc  Simple EM algorithm implementation for mixture models
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <numeric>
#include "em.h"
#include <iostream>
#include <cmath>

using namespace Portage;

double EM::count(const vector<double>& probs, Uint freq)
{
   double sum = 0.0;
   for (Uint i = 0; i < numModels(); ++i) {
      scratch[i] = weights[i] * probs[i];
      sum += scratch[i];
   }

   if (sum != 0.0)
      for (Uint i = 0; i < numModels(); ++i)
         counts[i] += freq * scratch[i] / sum;

   return sum;
}

/**
 * Estimate weights from counts.
 */
double EM::estimate(const vector<double>* prior_counts)
{
   if (prior_counts)
      for (Uint i = 0; i < counts.size(); ++i)
         counts[i] += (*prior_counts)[i];
   double delta = 0.0;
   double sum = accumulate(counts.begin(), counts.end(), 0.0);
   for (Uint i = 0; i < weights.size(); ++i) {
      double nw = counts[i] / sum;
      delta = max(delta, abs(nw - weights[i]));
      weights[i] = nw;
      counts[i] = 0.0;
   }
   return delta;
}
