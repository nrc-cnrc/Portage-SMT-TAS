/**
 * @author Michel Simard and George Foster
 * @file segmentmodel.h  Defines the interface for segmentation models used by
 * the BasicModel class. 
 * 
 * COMMENTS:
 *
 * A segmentation model captures the probability of splitting up a source
 * sentence into a sequence of phrases, regardless of how those phrases are
 * translated, or in what order. The advantage to grouping all segmentation
 * models here is that they share a particular profile of instantiations for 
 * DecoderFeature virtual functions, namely that the only mandatory function
 * they need to define is precomputeFutureScore(), which is also used for
 * score(). This is made explicit in the generic SegmentationModel interface
 * below.
 *
 * However, the assumption in this interface is that each source phrase (ie,
 * each segmentation decision at a particular point in the source sentence) is
 * independent of all others. This is reasonable, given that all segmentations
 * are conditioned on the whole (unsegmented) source sentence, but it isn't a
 * necessary characteristic of a segmentation model. If you do want to have
 * higher-order segment dependencies, you'll need to define some of the other
 * DecoderFeature functions.
 *
 * Any classes derived from this interface should be added to the create()
 * function, in segmentmodel.cc. Also, add documentation to canoe_help.h.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
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

    // The following default implementations apply to all segmentation models
    // (with the caveat above). You need only define precomputeFutureScore().

    virtual double score(const PartialTranslation& pt) {
      return precomputeFutureScore(*pt.lastPhrase);
    }
    virtual double partialScore(const PartialTranslation& trans) {return 0;}
    virtual Uint computeRecombHash(const PartialTranslation &pt) {return 0;}
    virtual bool isRecombinable(const PartialTranslation &pt1,
                                const PartialTranslation &pt2) {return true;}
    virtual double futureScore(const PartialTranslation &trans) {return 0;}
  };


  //--------------------------------------------------------------------------
  /**
   * "Phrase penalty" segmentation model: each phrase gets a constant score of
   * 1.
   */
  class SegmentCount : public SegmentationModel {
  public:
    virtual double precomputeFutureScore(const PhraseInfo& phrase_info) {
      return -1;
    }
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
    virtual double precomputeFutureScore(const PhraseInfo& phrase_info);
  };


}

#endif
