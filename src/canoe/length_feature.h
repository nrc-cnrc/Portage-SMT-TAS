/**
 * @author Eric Joanis
 * @file length_feature.h  Decoder feature: output sentence length
 *
 * $Id$
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef _LENGTH_FEATURE_H_
#define _LENGTH_FEATURE_H_

#include "phrasedecoder_model.h"
#include "decoder_feature.h"

namespace Portage{

   /// Decoder feature that basically returns the length of a translation as its score.
   class LengthFeature : public DecoderFeature {

   public:

      virtual double score(const PartialTranslation& pt) {
         return -double(pt.lastPhrase->phrase.size());
      }

      virtual Uint computeRecombHash(const PartialTranslation &pt) {
         return 0;
      }
      
      virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2) {
         return true;
      }

      virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
         return -double(phrase_info.phrase.size());
      }

      virtual double futureScore(const PartialTranslation &trans) {
         return 0;
      }

   };

}


#endif // _LENGTH_FEATURE_H_
