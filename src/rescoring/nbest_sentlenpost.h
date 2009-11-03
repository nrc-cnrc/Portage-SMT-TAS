/**
 * @author Nicola Ueffing
 * @file nbest_sentlenpost.h
 *
 * $Id$
 *
 *
 * COMMENTS: derived class for calculating sentence length posterior probabilities
 * over N-best lists.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/

#ifndef NBEST_SENTLENPOST_H
#define NBEST_SENTLENPOST_H

#include "nbest_posterior.h"

namespace Portage {

  /**
   * Elements of class NBestSentLentPost:
   * - Len2Posterior maps target sentence length to posterior probabilities
   */
  class NBestSentLenPost : public NBestPosterior {

  private:
    map<int, ConfScore> Len2Posterior;

  public:
    /// Constructor.
    NBestSentLenPost() {}
    /// Destructor.
    virtual ~NBestSentLenPost() {
      clearAll();
      Len2Posterior.clear();
    };

    virtual void   computePosterior(Uint src_sent_id);
    virtual void   normalizePosterior();
    virtual double sentPosteriorOne();
    virtual vector<double> wordPosteriorsOne();
    virtual void   tagPosteriorOne(ostream &out, int format=0);
    virtual void   tagPosteriorAll(ostream &out, int format=0);
    virtual void   tagSentPosteriorAll(ostream &out);
  };
}

#endif
