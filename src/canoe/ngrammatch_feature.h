/**
 * $Id$
 * @file ngrammatch_feature.h - 
 *   declaration of feature which is only used for (semi-)forced alignment of a 
 *   source-target sentence pair: 
 *   compute ngram match between a hypothesis and the actual target sentence
 *
 * PROGRAMMER: Nicola Ueffing
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef _NGRAMMATCH_FEATURE_H_
#define _NGRAMMATCH_FEATURE_H_

#include <map>
#include "ngram.h"
#include "decoder_feature.h"

namespace Portage {

  class NgramMatchFeature: public DecoderFeature {

   private:
      map<NGram<Uint>,int>   refNgrams;
      Uint                   refLen;
      const int              N;

   public:

      NgramMatchFeature(const string& args);


      void newSrcSent(const newSrcSentInfo& new_src_sent_info);

      /**
       * The optimistic score as calculated by maxNgramMatch(...)
       **/
      double precomputeFutureScore(const PhraseInfo& phrase_info);

      double futureScore(const PartialTranslation &trans);

      /**
       * The difference in score resulting from the addition of the last phrase
       *   to the partial translation.
       **/
      double score(const PartialTranslation& pt);

      /**
       * For complete translations, this is the actual ngram match with
       *   the reference sentence.
       * For partial translations, this is the optimistic score as calculated by
       *   maxNgramMatch(...)
       **/
      int ngramMatch(const PartialTranslation& pt);

      /**
       * For a given target phrase, determine the maximal ngram match with the
       * reference.  No complete coverage of the reference is required.
       * We thus obtain an optimistic score for partial translations.
       **/
      int maxNgramMatch(const Phrase& phr);

      Uint computeRecombHash(const PartialTranslation &pt);

      bool isRecombinable(const PartialTranslation &pt1,
                                  const PartialTranslation &pt2);
  
  }; // class NgramMatchFeature

} // Portage

#endif // _NGRAMMATCH_FEATURE_H_
