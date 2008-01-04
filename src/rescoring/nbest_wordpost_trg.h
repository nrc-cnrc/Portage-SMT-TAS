/**
 * @author Nicola Ueffing
 * @file nbest_wordpost_trg.h
 *
 * $Id$
 * 
 *
 * COMMENTS: derived class for calculating word posterior probabilities over N-best lists
 * based on the fixed target position
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/

#ifndef NBEST_WORDPOST_TRG_H
#define NBEST_WORDPOST_TRG_H

#include "nbest_posterior.h"

namespace Portage {

  /**
   * Elements of class NBestWordPostTrg:
   * - WordPos2Posterior maps pairs of target words and their target position to word posterior probabilities
   */
  class NBestWordPostTrg : public NBestPosterior {

  private:
    map<pair<Token,int>, ConfScore> WordPos2Posterior;
    
  public:
    /// Constructor.
    NBestWordPostTrg() {}
    /// Destructor.
    virtual ~NBestWordPostTrg() {
      clearAll();
      WordPos2Posterior.clear();
    };

    virtual void   computePosterior(const Uint src_sent_id);
    virtual void   normalizePosterior();
    virtual double sentPosteriorOne();
    virtual vector<double> wordPosteriorsOne();
    virtual void   tagPosteriorOne(ostream &out, const int &format=0);
    virtual void   tagPosteriorAll(ostream &out, const int &format=0);
    virtual void   tagSentPosteriorAll(ostream &out);
  };
}

#endif
