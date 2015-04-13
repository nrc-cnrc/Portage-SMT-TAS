/**
 * @author Eric Joanis
 * @file decoder_feature.h  Abstract parent class for all features used
 *                          in the decoder, except LMs and TMs.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef _DECODER_FEATURE_H_
#define _DECODER_FEATURE_H_

#include "portage_defs.h"
#include "phrasedecoder_model.h"
#include "new_src_sent_info.h"

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
       *        The returned DecoderFeature* MUST be valid aka not NULL.
       * @param bmg
       * @param group  name of derived type group (sub class)
       * @param args   argument string for derived constructor or name of sub sub class, if any.
       * @param fail   if true, die with error message when problems occur on
       *               construction 
       * @param verbose standard verbosity levels
       * @return the new feature, which the caller should free with delete when
       *        it is no longer needed
       */
      static DecoderFeature* create(BasicModelGenerator* bmg,
                                    const string& group,
                                    const string& args, bool fail = true,
                                    Uint verbose = 0);

      /**
       * Calculate the total size of memory mapped files in feature group:args,
       * if any.
       * @param group  name of derived type group (sub class)
       * @param args   argument string for derived constructor or name of sub sub class, if any.
       * @return total size of memory mapped files associated with the feature; 0
       *         in case of problems or if the model does not use memory mapped IO.
       */
      static Uint64 totalMemmapSize(const string& group, const string& args);

      /**
       * Finalize the initialization of a feature.  create() is called before
       * TMs and LMs are loaded, whereas finalizeInit() is call after TMs and
       * LMs are loaded.  If you features needs information from the TMs or LMs
       * to load properly, do that work here instead of in your constructor.
       */
      virtual void finalizeInitialization() {}

      /**
       * Initialize the feature on a new sentence.
       *
       * This function is called by the decoder before translating each source
       * sentence.  
       * @param info All infos related to source/target sentence.
       *             See newSrcSentInfo.
       */
      virtual void newSrcSent(const newSrcSentInfo& info) {}

      /**
       * Precompute this feature's score for a single phrase pair.
       * 
       * Compute the highest score for the given source range and translation,
       * independent of target context. Return 0 if the context-independent
       * computation can't be factored out for this feature. This function will
       * be called by BasicModelGenerator::precomputeFutureScores() when it
       * applies the dynamic programming technique of precomputing future
       * scores.
       *
       * @param phrase_info phrase pair for which to precompute
       * @return precomputed score
       */
      virtual double precomputeFutureScore(const PhraseInfo& phrase_info) = 0;

      /**
       * Compute this feature's future score for a partial translation.
       *
       * Compute the highest score that this feature can assign to any complete
       * extension of the given partial translation (scoring the extension
       * only). This should not duplicate any scoring already accounted for by
       * precomputeFutureScore(), since the final future score will be the sum
       * of what the two methods provide.  This function will be called for
       * each feature by BasicModel::computeFutureScore().
       *
       * Note: it is an error to return a non-zero future score for a complete
       * translation.
       *
       * @param pt partial translation for which to produce a future score
       * @return the future score
       */
      virtual double futureScore(const PartialTranslation &pt) = 0;

      /**
       * Compute the score for adding a new phrase pair to a partial
       * translation.
       *
       * The score returned should be the marginal cost of the last phrase
       * added, pt.lastPhrase, not the score of the whole partial translation.
       *
       * @param pt the partial translation to score
       * @return the score
       */
      virtual double score(const PartialTranslation& pt) = 0;

      /**
       * Partial score based only on the source range, not the target phrase.
       *
       * Compute as much of this feature's marginal score as can be inferred
       * taking into consideration pt.lastPhrase's src_words, but *ignoring*
       * the target words in pt.lastPhrase!
       *
       * The sum partialScore(pt) + precomputeFutureScore(pt.lastPhrase)
       * is used by cube pruning as a heuristic for score(pt), so those two
       * methods should not take into account the same information.
       *
       * In the base class, we return 0, because it is expected that most
       * features can't do anything with only the last source range.  Should be
       * overriden in subclasses where this is not the case, like distortion.
       *
       * @param pt           previous partial translation
       * @return inferrable part of score()
       */
      virtual double partialScore(const PartialTranslation &pt)
      { return 0.0; }

      /**
       * Compute this feature's partial future score, for cube pruning decoding.
       *
       * This method is like futureScore(), except it's now allowed to take into
       * account pt.lastPhrase's target words. Just like partialScore(), this
       * is used by cube pruning for heuristically calculating the future score
       * when knowing pt.lastPhrase's src_words, but before deciding which target
       * phrase will be used to translate those source words.
       *
       * The default implementation calls futureScore(); override only if your
       * futureScore() implementation needs pt.lastPhrase->phrase or other
       * target phrase dependant information.
       */
      virtual double partialFutureScore(const PartialTranslation &pt)
      { return futureScore(pt); }

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
       * Consider a new phrase p added to pt1 and pt2 to form new partial
       * translation pt1' and pt2'.  Partial translations pt1 and pt2 are
       * recombinable if for every p, score(pt1') == score(pt2').
       *
       * This should capture information that is only pertinent to the feature
       * being implemented, i.e. NOT the last two target words, nor the set of
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
       * Special case for features that rely on the previous words.
       *
       * If your feature relies on the identity of the previous n words, such
       * as n-gram LM models do, implement this method to return the number of
       * words of context your feature needs from the previous decoder state,
       * instead of considering that context in isRecombinable() and
       * computeRecombHash().
       *
       * This way, the test will be done efficiently by BasicModel itself,
       * over the maximum context required by all features and LMs.
       */
      virtual Uint lmLikeContextNeeded() { return 0; }

      /**
       * @brief Get a human readable description of the feature.
       *
       * By default, this description puts together the group, name and args
       * parameters passed to create().
       * To override this default value for your derived class, reset the
       * protected member "description" above to a new value in your create()
       * function or in your constructor.
       *
       * WARNING: not virtual!  Do not try to override!
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

