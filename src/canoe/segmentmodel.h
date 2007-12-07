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
 * Technologies langagieres interactives / Interactive Language Technologies
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
     * @param name_and_arg name of derived type, with optional argument
     *                     introduced by # if appropriate.
     * @param fail die with error message if true and problems occur on
     * construction 
     * @return new model; free with delete
     */
    static SegmentationModel* create(const string& name_and_arg,
          bool fail = true);

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
