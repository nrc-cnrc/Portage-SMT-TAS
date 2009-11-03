/**
 * @author Nicola Ueffing
 * @file nbest_wordpost_lev.h
 *
 * $Id$
 *
 *
 * COMMENTS: derived class for calculating word posterior probabilities over N-best lists
 * based on the Levenshtein-aligned target position (usually best method for WER-based classification)
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/

#ifndef NBEST_WORDPOST_LEV_H
#define NBEST_WORDPOST_LEV_H

#include "nbest_posterior.h"

namespace Portage {

  /** Calculate word posterior probabilities based on Levenshtein alignment
   * over N-best lists. The probablities of all sentences containing a word in
   * a position Levenshtein-aligned to itself are summed, and this sum is
   * normalized by the probability mass of the whole N-best list.
   */
  class NBestWordPostLev : public NBestPosterior {

  private:
    /// store confidence value for each target word
    vector<ConfScore> conf;

  public:
    /// Constructor.
    NBestWordPostLev() {}
    /// Destructor.
    virtual ~NBestWordPostLev() {
      clearAll();
    };

    virtual void   init(const Tokens &t);
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
