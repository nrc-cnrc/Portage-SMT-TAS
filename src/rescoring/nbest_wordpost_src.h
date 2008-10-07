/**
 * @author Nicola Ueffing
 * @file nbest_wordpost_src.h
 *
 * $Id$
 *
 *
 * COMMENTS: derived class for calculating word posterior probabilities over N-best lists
 * based on the source phrase to which the target word is aligned
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/

#ifndef NBEST_WORDPOST_SRC_H
#define NBEST_WORDPOST_SRC_H

#include "nbest_posterior.h"

namespace Portage {

  struct wordSrcPhraseLessThan {
    bool operator()(const pair<Token,PhraseRange> &p1, const pair<Token,PhraseRange> &p2) {
      if (p1.second.first < p2.second.first) return true;
      if (p1.second.first > p2.second.first) return false;
      if (p1.second.last  < p2.second.last)  return true;
      if (p1.second.last  > p2.second.last)  return false;
      if (p1.first < p2.first) return true;
      return false;
    }
  };

  /**
   * Elements of class NBestWordPostSrc:
   * - WordSrcPhrase2Posterior maps pairs of target words and their aligned source phrase to word posterior probabitlities
   * - alig is the phrase alignment of the target sentence
   */
  class NBestWordPostSrc : public NBestPosterior {

  private:
    map< pair<Token,PhraseRange>, ConfScore, wordSrcPhraseLessThan > WordSrcPhrase2Posterior;
    Alignment alig;

  public:
    /// Constructor.
    NBestWordPostSrc() {}
    /// Destructor.
    virtual ~NBestWordPostSrc() {
      clearAll();
      WordSrcPhrase2Posterior.clear();
    };

    virtual void   setAlig(Alignment &al);

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
