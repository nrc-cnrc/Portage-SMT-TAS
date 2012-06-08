/**
 * $Id$
 * @file levenshtein_feature.h - 
 *   declaration of feature which is only used for (semi-)forced alignment of a 
 *   source-target sentence pair: 
 *   compute Levenshtein distance between a hypothesis and the actual target sentence
 *
 * PROGRAMMER: Nicola Ueffing
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef _LEVENSHTEIN_FEATURE_H_
#define _LEVENSHTEIN_FEATURE_H_

#include "decoder_feature.h"
#include "levenshtein.h"

namespace Portage {

  class LevenshteinFeature: public DecoderFeature {

   private:
      vector<Uint>       ref;
      Levenshtein<Uint>  lev;
      int                levLimit; 
      int                relLevLimit; // relative to src_sent length
      Uint               verbosity;
  
   public:
   
      /**
       * Contructor with the levenshtien Limit passed on
       **/
      LevenshteinFeature(BasicModelGenerator* bmg);

      void newSrcSent(const newSrcSentInfo& new_src_sent_info);

      /**
       * The optimistic score as calculated by minLevDist(...)
       **/
      double precomputeFutureScore(const PhraseInfo& phrase_info);

      double futureScore(const PartialTranslation &trans);

      /**
       * The difference in score resulting from the addition of the last phrase
       *   to the partial translation.
       **/
      double score(const PartialTranslation& pt);

      /**
       * For complete translations, this is the actual Levenshtein distance
       *   to the reference sentence.
       * For partial translations, this is the optimistic score as calculated by
       *   partialLevDist(...)
       **/
      int levDist(const PartialTranslation& pt);

      /**
       * For a given target phrase, find the part of the reference (not necessarily
       * starting at the beginning) that has the minimal Levenshtein distance. 
       * No complete coverage of the reference (towards sentence start or end) is required.
       * We thus obtain a very optimistic score for a target phrase.
       **/
      int minLevDist(const VectorPhrase& phr);

      /**
       * For a given partial translation, determine Levenshtein distance from 
       * the sentence start on. No complete coverage of the reference (towards the 
       * sentence end) is required.
       * We thus obtain an optimistic score for partial translations, and we return the
       * set of min positions.
       **/
      int partialLevDist(const VectorPhrase& hyp, boost::dynamic_bitset<>* min_positions);

      Uint computeRecombHash(const PartialTranslation &pt);

      /**
       * Levenshtein recombination: Check whether the 2 partial translations
       * have the same (partial) Lev. distance to the ref. and wether this
       * minimal distance is obtained by covering the same part of the ref.
       **/
      bool isRecombinable(const PartialTranslation &pt1,
                                  const PartialTranslation &pt2);

   }; // class LevenshteinFeature

} // Portage

#endif // _LEVENSHTEIN_FEATURE_H_

