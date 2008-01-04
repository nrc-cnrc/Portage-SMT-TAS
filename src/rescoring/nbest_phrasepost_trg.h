/**
 * @author Nicola Ueffing
 * @file nbest_phrasepost_trg.h
 *
 * $Id$
 * 
 *
 * COMMENTS: derived class for calculating phrase posterior probabilities over N-best lists
 * based on the fixed target position (of the phrase in the sentence)
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/

#ifndef NBEST_PHRASEPOST_TRG_H
#define NBEST_PHRASEPOST_TRG_H

#include "nbest_posterior.h"

namespace Portage {

  /** 
   * Compare two pairs of Tokens (target phrase) and int (sentence position in which the phrase occurs)
   */
  struct phrasePosLessThan {
    bool operator()(const pair<Tokens,int> &p1, const pair<Tokens,int> &p2) {
      if (p1.first.size() < p2.first.size()) return true;
      if (p1.first.size() > p2.first.size()) return false;
      if (p1.second < p2.second) return true;
      if (p1.second > p2.second) return false;
      for (uint i=0; i<p1.first.size(); i++) {
	if (p1.first[i] < p2.first[i]) return true;
	if (p1.first[i] > p2.first[i]) return false;
      }
      return false;
    }
  };

  /**
   * Elements of class NBestPhrasePostTrg:
   * - PhrasePos2Posterior maps pairs of target phrases and their target position to phrase posterior probabilities
   * - alig is the phrase alignment of the target sentence
   */
  class NBestPhrasePostTrg : public NBestPosterior {

  private:
    map<pair<Tokens,int>, ConfScore, phrasePosLessThan> PhrasePos2Posterior;
    Alignment alig;
    
  public:
    /// Constructor.
    NBestPhrasePostTrg() {}
    /// Destructor.
    virtual ~NBestPhrasePostTrg() {
      clearAll();
      PhrasePos2Posterior.clear();
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
