/**
 * @author Eric Joanis
 * @file decoder_feature.h  Abstract parent class for all features used
 *                          in the decoder, except LMs and TMs.
 *
 * $Id$
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Conseil national de recherches du Canada / 
 * Copyright 2006, National Research Council of Canada
 */

#ifndef _DECODER_FEATURE_H_
#define _DECODER_FEATURE_H_

#include <portage_defs.h>
#include "phrasedecoder_model.h"

namespace Portage {

class BasicModelGenerator;

   /**
    * @brief Base class for all decoder features.
    *
    * Abstract class that each feature used in the decoder should derive.
    * Each feature can be an actual log prob model, but not necessarily.
    *
    * To add a new type of feature, you need to:
    *
    * 1) Implement all pure virtual functions: precomputeFutureScore(),
    *    futureScore(), score(), computeRecombHash() and isRecombinable()
    *    (see each function's description for details).
    *
    * 2) Give your subclass a constructor (if it is simple) or a create()
    *    function (if it has sub sub classes).
    *
    * 3) Add the necessary call to your constructor or create() function to
    *    create() in decoder_feature.cc.
    *
    * 4) Update config_io.h and .cc so that the decoder and related utilities
    *    are able to read the parameters your feature needs from the canoe.ini
    *    file and/or the command line.
    *
    * 5) Update BasicModelGenerator::InitDecoderFeatures() to create your
    *    features when appropriate.
    */
   class DecoderFeature {
   protected:
      /**
       * @brief Default human-readable decription based on create arguments.
       *
       * Replace this value in your sub-class's create() function or
       * constructor if you want to produce a description different from the
       * default (see describeFeature() for details).
       */
      string description;

   public:

      /**
       * @brief Virtual constructor: creates a designated derived class with
       *        given arguments.
       * @param bmg   
       * @param group  name of derived type group (sub class)
       * @param name   name of derived type (sub sub class, if any)
       * @param args   argument string for derived constructor.
       * @param fail   if true, die with error message when problems occur on
       *               construction 
       * @return the new feature, which the caller should free with delete when
       *        it is no longer needed
       */
      static DecoderFeature* create(BasicModelGenerator* bmg,
                                    const string& group, const string& name,
                                    const string& args, bool fail = true);
    
      /**
       * Initialize the feature on a new sentence.
       *
       * This function is called by the decoder before translating each source
       * sentence.  
       * @param src_sent words in source sentence, with markup stripped
       * @param phrase_infos A triangular array of all the phrase options for
       * the source sentence. The (i, j)-th entry of the array contains all
       * phrase translation options for the source range [i, i + j + 1).
       */
      virtual void newSrcSent(const vector<string>& src_sent,
                              vector<PhraseInfo *>** phrase_infos) {}

      /**
       * Set the current partial translation context.
       *
       * The decoder calls this method to inform the feature of the current
       * PartialTranslation context.  Between calls to this function, the
       * "<trans>" arguments to BasicModel::scoreTranslation() and
       * BasicModel::computeFutureScore() (and therefore score() and
       * futureScore() below) are guaranteed to differ only in their lastPhrase
       * members, ie in their last source/target phrase pairs, having their
       * back pointer all point to the partial translation argument passed
       * here.  The purpose is to give the feature a chance to factor out
       * computations that depend only on this pair.
       * 
       * @param trans the current partial translation context
       */
      virtual void setContext(const PartialTranslation& trans) {}

      /**
       * Precompute this feature's future score for a single phrase.
       *
       * Precompute the future score based on a given contiguous input range
       * and suggested translation.  Currently, it is assumed that precomputed
       * future score for any union of disjoint ranges is simply the sum of
       * those precomputed future scores.  Breaking this assumption may imply
       * deep changes in the decoder, so try not to break it!
       * This function will be called for each feature by
       * BasicModelGenerator::precomputeFutureScores() when it applies the
       * dynamic programming technique of precomputing future scores.  It is
       * called for a given source phrase and a candidate target phrase to
       * translate it.
       *
       * @param phrase_info the phrase for which to precompute a future score
       * @return the precomputed future score
       */
      virtual double precomputeFutureScore(const PhraseInfo& phrase_info) = 0;

      /**
       * @brief Compute this feature's future score for a partial translation.
       *
       * Compute the future score based on a current partial translation
       * Any aspects of the future score that apply to contiguous
       * source ranges are handled by precomputeFutureScore(), whereas this
       * function can deal with aspects of the future score that are the result
       * of discontinuous non-convered ranges in a partial translation.
       * This function will be called for each feature by
       * BasicModel::computeFutureScore() to add information to the precomputed
       * future score, and should not duplicate any scoring already accounted
       * for by precomputeFutureScore(), since the final future score will be
       * the sum of what the two methods provide.
       *
       * @param trans partial translation for which to produce a future score
       * @return the future score
       */
      virtual double futureScore(const PartialTranslation &trans) = 0;

      /**
       * Compute the marginal score a partial translation.
       * The score returned should be the marginal cost of the last phrase
       * added, pt.lastPhrase, not the score of the whole partial translation.
       *
       * Assign a score to a particular hypothesis according to your feature.
       * By convention, higher scores indicate better hypotheses.
       *
       * @param pt the partial translation to score
       * @return the score
       */
      virtual double score(const PartialTranslation& pt) = 0;

      /**
       * Compute a recombining hash value.
       *
       * The hash value returned should capture information that is only
       * pertinent to the feature being implemented, ie NOT the last two target
       * words, nor the set of covered source words, nor anything that is not
       * specific to this feature.
       *
       * @param pt partial translation for which to compute the hash
       * @return a hash value such that isRecombinable(pt1,pt2) implies
       *         computeRecombHash(pt1) == computeRecombHash(pt2) (but not
       *         necessarily vice-versa)
       */
      virtual Uint computeRecombHash(const PartialTranslation &pt) = 0;

      /**
       * @brief Determine if two partial translations are recombinable or not.
       *
       * This should capture information that is only pertinent to the feature
       * being implemented, ie NOT the last two target words, nor the set of
       * covered source words, nor anything that is not specific to this
       * feature.
       *
       * @param pt1 first partial translation
       * @param pt2 second partial translation
       * @return true iff pt1 and pt2 are recombinable with respect to this
       *         feature
       */
      virtual bool isRecombinable(const PartialTranslation &pt1,
                                  const PartialTranslation &pt2) = 0;


      /**
       * @brief Get a human readable description of the feature.
       *
       * By default, this description puts together the group, name and args
       * parameters passed to create().
       * To override this default value for your derived class, reset the
       * protected member "description" above to a new value in your create()
       * function or in your constructor.
       *
       * @return A short string describing the feature and its parameters
       */
      string describeFeature() const { return description; }

      /**
       * @brief Destructor
       *
       * This destructor is virtual since this class is intended to be derived
       */
      virtual ~DecoderFeature() {}

   }; // class DecoderFeature

} // Portage

#endif // _DECODER_FEATURE_H_

