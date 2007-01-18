/**
 * @author Michel Simard
 * @file segmentmodel.h  Defines the interface for segmentation models used by
 * the BasicModel class. 
 * 
 * COMMENTS:
 *
 * Any classes derived from this interface should be added to the create()
 * function, in segmentmodel.cc.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef SEGMENTMODEL_H
#define SEGMENTMODEL_H

#include <map>
#include "phrasedecoder_model.h"
#include "decoder_feature.h"

namespace Portage {

  /**
   * Interface for segmentation model decoder features.
   */
  class SegmentationModel : public DecoderFeature {

  public:

    /**
     * Virtual constructor: creates a designated derived class with given
     * arguments.
     * @param name name of derived type
     * @param args argument string for derived constructor.
     * @param fail die with error message if true and problems occur on
     * construction 
     * @return new model; free with delete
     */
    static SegmentationModel* create(const string& name, const string& args, bool fail = true);

    /**
     * This function is called by the decoder before translating each source
     * sentence.  
     * @param src_sent words in source sentence, with markup stripped
     * @param phrase_infos A triangular array of all the phrase options for the
     * source sentence. The (i, j)-th entry of the array contains all phrase
     * translation options for the source range [i, i + j + 1).
     */
    //virtual void newSrcSent(const vector<string>& src_sent, vector<PhraseInfo *>** phrase_infos) {}

    /**
     * A call to inform the model of the current PartialTranslation context.  
     * Between calls to this function, the <trans> arguments to scoreTranslation() 
     * and computeFutureScore() are guaranteed to differ only in their lastPhrase 
     * members, ie in their last source/target phrase pairs. The purpose is to give
     * the model a chance to factor out computations that depend only on this pair.
     */
    //virtual void setContext(const PartialTranslation& trans) {}

    /**
     * Assign a segmentation score to a particular hypothesis. By convention,
     * higher scores indicate better hypotheses.
     */
    //virtual double score(const PartialTranslation& pt) = 0;

    /**
     * Compute a hash value for given hypothesis. This should capture 
     * information that is only pertinent to the distortion model, ie NOT the
     * last two target words, nor the set of covered source words.
     */
    //virtual Uint computeRecombHash(const PartialTranslation &pt) = 0;

    /**
     * Determine if two partial translations are recombinable or not. This
     * should capture information that is only pertinent to the distortion
     * model, ie NOT the last two target words, nor the set of covered source
     * words.
     */
    //virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2) = 0;
  };


  //--------------------------------------------------------------------------
  /**
   * "Phrase penalty" segmentation model: each phrase gets a constant score of
   * 1.
   */
  class SegmentCount : public SegmentationModel {

  public:

    virtual double score(const PartialTranslation& pt);
    virtual Uint computeRecombHash(const PartialTranslation &pt);
    virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2);
    virtual double precomputeFutureScore(const PhraseInfo& phrase_info);
    virtual double futureScore(const PartialTranslation &trans);
  };

  //--------------------------------------------------------------------------
  /**
   * Bernoulli segmentation model: probability to segment source at any point
   * is a constant P.
   */
  class BernoulliSegmentationModel : public SegmentationModel {
    double boundary;      ///< Q
    double not_boundary;  ///< 1-Q

  public:
    /** 
     * Constructor.
     * @param arg  which represents Q the probability of success.
     */
    BernoulliSegmentationModel(const string &arg);
    virtual double score(const PartialTranslation& pt);
    virtual Uint computeRecombHash(const PartialTranslation &pt);
    virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2);
    virtual double precomputeFutureScore(const PhraseInfo& phrase_info);
    virtual double futureScore(const PartialTranslation &trans);
  };


}

#endif
