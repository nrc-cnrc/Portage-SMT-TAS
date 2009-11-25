/**
 * @author George Foster
 * @file em.h  Simple EM algorithm implementation for mixture models
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#ifndef __EM_H__
#define __EM_H__

#include <vector>
#include "portage_defs.h"

namespace Portage
{

/**
 * Use this when you have a set of models, and you want to estimate weights for
 * them from some training data. Iterate over the data some number of times,
 * calling count() on each instance, then estimate() at the end of each
 * iteration. There's an example in tests/test_em.h.
 */
class EM {

   vector<double> counts;
   vector<double> weights;
   vector<double> scratch;

public:

   /**
    * Construct for a given number of models, initializing weights to uniform
    * values.  
    * @param num_models
    */
   EM(Uint num_models) : 
      counts(num_models), 
      weights(num_models, 1.0 / num_models),
      scratch(num_models) {}

   /**
    * Count one training instance.
    * @params probs vector[numModels()] of probabilities assigned by models
    * to this instance
    * @return the probability of the instance under current weights
    */
   double count(const vector<double>& probs);
   
   /**
    * Estimate weights from counts. The M-step.
    * @return absolute value of largest single-weight delta
    */
   double estimate();

   /**
    * Return number of models.
    */
   Uint numModels() {return weights.size();}

   /**
    * Get current weights.
    */
   vector<double>& getWeights() {return weights;}
   
};

} // Portage

#endif // __EM_H__
