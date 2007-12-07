/**
 * @author George Foster
 * @file distortionmodel.h  Defines the interface for distortion models used by
 * the BasicModel class. Also contains the default penalty-based distortion
 * model, called WordDisplacement.
 * 
 * COMMENTS:
 *
 * Any classes derived from this interface should be added to the create()
 * function, in distortionmodel.cc.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef DISTORTIONMODEL_H
#define DISTORTIONMODEL_H

#include "phrasedecoder_model.h"
#include "decoder_feature.h"

namespace Portage {

/// Common class for distortion model decoder features.
class DistortionModel : public DecoderFeature
{
public:

   /**
    * Virtual constructor: creates a designated derived class with given
    * arguments.
    * @param name_and_arg name of derived type, with optional argument
    *                     introduced by # if appropriate.
    * @param fail die with error message if true and problems occur on
    * construction 
    * @return new model; free with delete
    */
   static DistortionModel* create(const string& name_and_arg,
         bool fail = true);
};


/**
 * Basic distortion model: word displacement penalty
 */
class WordDisplacement : public DistortionModel
{
protected:
   Uint sentLength;  ///< Sentence length

public:

   virtual void newSrcSent(const vector<string>& src_sent, vector<PhraseInfo *>** phrase_infos) {
      sentLength = src_sent.size();
   }

   virtual double score(const PartialTranslation& pt);

   virtual Uint computeRecombHash(const PartialTranslation &pt);

   virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2);

   virtual double precomputeFutureScore(const PhraseInfo& phrase_info);

   virtual double futureScore(const PartialTranslation &trans);

};

/**
 * Uninformative distortion model: returns 0 on partial translations, 1 on
 * completed.
 * Here's the motivation for that choice.  We want a model that
 * essentially functions as a no-op, one that model optimization will
 * want to ignore (set weight to zero).
 *
 * - if the model returns a constant value per partial translation,
 *   then it behaves like a phrase penalty model, and therefore is not
 *   uninformative. 
 * - if the model always returns 0, then its model parameter is free,
 *   which is bad for optimization;
 * - if the model returns a random value, then it has a somewhat
 *   unpredictable behavior, which appears to be problematic,
 *   especially for small data sets.
 * - so a model that returns a non-null value, constant over all
 *   translations appears to be the way to go.
 *   
 */
class ZeroInfoDistortion : public DistortionModel
{
public:

   virtual void newSrcSent(const vector<string>& src_sent, vector<PhraseInfo *>** phrase_infos) {}

   virtual double score(const PartialTranslation& pt) {
     return pt.sourceWordsNotCovered.empty() ? 1.0 : 0.0;
   }

   virtual Uint computeRecombHash(const PartialTranslation &pt) {
     return 0;
   }

   virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2) { 
     return true; 
   }

   virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
     return 0;
   }

   virtual double futureScore(const PartialTranslation &trans) {
     return 1;
   }
};


}

#endif
