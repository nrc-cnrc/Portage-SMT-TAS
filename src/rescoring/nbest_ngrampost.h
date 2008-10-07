/**
 * @author Nicola Ueffing
 * @file nbest_ngrampost.h  derived class for calculating n-gram posterior
 * probabilities over N-best lists.
 *
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/

#ifndef NBEST_NGRAMPOST_H
#define NBEST_NGRAMPOST_H

#include "nbest_posterior.h"
#include "ngram_tree.h"

namespace Portage {

   /**
    * Elements of class NBestNgramPost:
    * - Ngram2Posterior is a tree mapping n-grams to their posterior probabilities
    * - maxN is the maximal n-gram length which is considered
    */
   class NBestNgramPost : public NBestPosterior {

   private:
      NgramTree Ngram2Posterior;
      Uint      maxN;

   public:
      /// Constructor.
      NBestNgramPost() : Ngram2Posterior(NgramTree()) {}
      /// Destructor.
      virtual ~NBestNgramPost() {
         clearAll();
         Ngram2Posterior.clear();
      };

      virtual void   computePosterior(Uint src_sent_id);
      virtual void   normalizePosterior();
      virtual double sentPosteriorOne();
      virtual vector<double> wordPosteriorsOne();
      virtual void   tagPosteriorOne(ostream &out, int format=0);
      virtual void   tagPosteriorAll(ostream &out, int format=0);
      virtual void   tagSentPosteriorAll(ostream &out);

      virtual void   setMaxN(Uint m) {maxN = m;}
      Uint   getMaxN() {return maxN;}

   };
}

#endif
