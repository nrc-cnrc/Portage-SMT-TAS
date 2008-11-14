/**
 * @author George Foster
 * @file ibm_feature.h  IBM-based feature functions for canoe.
 * 
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef IBM_FEATURE_H
#define IBM_FEATURE_H

#include <stdlib.h>
#include <ibm.h>
#include <str_utils.h>
#include "decoder_feature.h"

class BasicModelGenerator;

namespace Portage {

/// forward IBM1 decoder feature
class IBM1FwdFeature : public DecoderFeature
{
   /// much smaller than for phrasetables, because values down to 1e-30 occur in
   /// ttables 
   static const double LOG_ALMOST_ZERO = -70.0;

   BasicModelGenerator* bmg;  ///< to get all targets for S
   IBM1* ibm1;                ///< to get Pr(S|T)

   QuickSet active_voc;      ///< for current sentence
   vector<double> logprobs;  ///< for each word in active voc

   /**
    * Calculates the log probability for a phrase.
    * @param phrase  phrase to calculate its log prob from the active vocabulary
    * @return Returns the log probability for a phrase
    */
   double phraseLogProb(const Phrase& phrase);
   
public:

   /**
    * Constructor.
    * @param bmg
    * @param modelfile
    */
   IBM1FwdFeature(BasicModelGenerator* bmg, const string& modelfile);

   virtual void newSrcSent(const newSrcSentInfo& new_src_sent_info);

   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
      return phraseLogProb(phrase_info.phrase);
   }

   virtual double score(const PartialTranslation& pt) {
      return phraseLogProb(pt.lastPhrase->phrase);
   }

   virtual double partialScore(const PartialTranslation &trans) {return 0.0;}
   virtual double futureScore(const PartialTranslation &trans) {return 0.0;}
   virtual Uint computeRecombHash(const PartialTranslation &pt) {return 0;}
   virtual bool isRecombinable(const PartialTranslation &pt1,
                       const PartialTranslation &pt2) {return true;}

   /// Destructor.
   ~IBM1FwdFeature() {
      // delete ibm1;
   }
};


}

#endif
