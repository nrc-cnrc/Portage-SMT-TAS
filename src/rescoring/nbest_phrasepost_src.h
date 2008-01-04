/**
 * @author Nicola Ueffing
 * @file nbest_phrasepost_src.h
 *
 * $Id$
 * 
 *
 * COMMENTS: derived class for calculating phrase posterior probabilities over N-best lists
 * based on the aligned source phrase
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/

#ifndef NBEST_PHRASEPOST_SRC_H
#define NBEST_PHRASEPOST_SRC_H

#include "nbest_posterior.h"

namespace Portage {

  /** 
   * Compare two pairs of Tokens (target phrase) and PhraseRange (aligned source phrase)
   */
  struct phrasePairLessThan {
    bool operator()(const pair<Tokens,PhraseRange> &p1, const pair<Tokens,PhraseRange> &p2) {
      if (p1.first.size() < p2.first.size()) return true;
      if (p1.first.size() > p2.first.size()) return false;
      if (p1.second.first < p2.second.first) return true;
      if (p1.second.first > p2.second.first) return false;
      if (p1.second.last  < p2.second.last)  return true;
      if (p1.second.last  > p2.second.last)  return false;
      for (uint i=0; i<p1.first.size(); i++) {
	if (p1.first[i] < p2.first[i]) return true;
	if (p1.first[i] > p2.first[i]) return false;
      }
      return false;
    }
  };

  /**
   * Elements of class NBestPhrasePostSrc:
   * - PhrasePair2Posterior maps pairs of target phrases and their aligned source phrase to posterior probabilities
   * - alig is the phrase alignment of the target sentence
   */ 
  class NBestPhrasePostSrc : public NBestPosterior {

  private:
    map< pair<Tokens,PhraseRange>, ConfScore, phrasePairLessThan > PhrasePair2Posterior;
    Alignment alig;
    
  public:
    /// Constructor.
    NBestPhrasePostSrc() {}
    /// Destructor.
    virtual ~NBestPhrasePostSrc() {
      clearAll();
      PhrasePair2Posterior.clear();
    };

    virtual void   setAlig(Alignment &al);

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
