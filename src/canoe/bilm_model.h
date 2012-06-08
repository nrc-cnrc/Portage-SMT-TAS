/**
 * @author Eric Joanis
 * @file bilm_model.h  BiLM decoder feature, following Niehues et al, WMT-2011.
 *
 * $Id$
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2012, Her Majesty in Right of Canada
 */

#ifndef _BILM_MODEL_H_
#define _BILM_MODEL_H_

#include "decoder_feature.h"


namespace Portage {

class PLM;
class VocabFilter;

class BiLMModel : public DecoderFeature {
   PLM* bilm;
   Uint order;
   VocabFilter& voc;
   Uint sentStartID;
   void getLastBiWordsBackwards(VectorPhrase &biWords, Uint num, const PartialTranslation& trans);
public:
   BiLMModel(BasicModelGenerator* bmg, const string& filename);
   ~BiLMModel();
   virtual void newSrcSent(const newSrcSentInfo& info);
   virtual double precomputeFutureScore(const PhraseInfo& phrase_info);
   virtual double futureScore(const PartialTranslation &trans);
   virtual double score(const PartialTranslation& pt);
   virtual double partialScore(const PartialTranslation &trans);
   virtual Uint computeRecombHash(const PartialTranslation &pt);
   virtual bool isRecombinable(const PartialTranslation &pt1,
                               const PartialTranslation &pt2);
}; // BiLMModel

} // Portage


#endif // _BILM_MODEL_H_
