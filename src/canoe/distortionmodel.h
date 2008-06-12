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

   /**
    * Verify whether a coverage and last phrase violate the distortion limit.
    *
    * Unlike most decoder features, distortion not only has a cost, but also
    * a pruning behaviour when the distortion limit is exceeded.  We implement
    * the test for the distortion limit here in one centralized place, for
    * use in the several places where it is needed.
    *
    * With distLimitExt=false, the distortion limit implemented is more
    * conservative than what is normally understood: a jump from last word in
    * new_phrase back to the first non-covered word in cov in a single step has
    * to be possible for this function to return true.  This is what we used
    * form the beginning in canoe, it is what Bob Moore uses in his system
    * (according to his 2007 paper), and probably what Pharaoh and Moses do,
    * though this has not been verified.
    *
    * With distLimitExt=true, the implementation is still not the full normally
    * understood definition, but it is less conservative.  In this case, a
    * phrase is allowed to end too far to jump back to the first non-covered
    * word in cov, as long as 1) it starts within the distortion limit from the
    * first non-covered word in cov, and 2) there exists a sequence of jumps
    * that respects the distortion limit and can complete the sentence.

    * @param cov Coverage set (i.e., words not translated yet) of previous
    *   partial translation, not including new_phrase (i.e., the range
    *   new_phrase must be in cov as words not translated yet).
    * @param last_phrase source range of the last phrase covered
    * @param new_phrase source range of the candidate phrase to add
    * @param distLimit Maximum allowable distortion
    * @param sourceLength lenght of the source sentence
    * @param distLimitExt whether to use "extended" distortion limit
    * @param resulting_cov The coverage that results in adding new_phrase to
    * cov - pass it in only if you already needed to calculate it, in which
    * case if this method needs it, it won't re-calculate it.
    * @pre !cov.empty(), i.e., there must be some non-covered words, in
    *   particular, new_phrase must not be covered.
    * @return whether the distortion limit is respected if new_phrase is added.
    */
   static inline bool respectsDistLimit(const UintSet& cov,
         Range last_phrase, Range new_phrase, int distLimit, Uint sourceLength,
         bool distLimitExt, const UintSet* resulting_cov = NULL)
   {
      if ( distLimitExt )
         return respectsDistLimitExt(cov, last_phrase, new_phrase, distLimit,
               sourceLength, resulting_cov);
      else
         return respectsDistLimitCons(cov, last_phrase, new_phrase, distLimit,
               sourceLength, resulting_cov);
   }

   /**
    * Check if adding new_phrase corresponds to swapping contiguous phrases or
    * to monotonic decoding.
    *
    * @param cov Coverage set (i.e., words not translated yet) of previous
    *   partial translation, not including new_phrase (i.e., the range
    *   new_phrase must be in cov as words not translated yet).
    * @param last_phrase source range of the last phrase covered
    * @param new_phrase source range of the candidate phrase to add
    * @param sourceLength lenght of the source sentence
    * @param phrases the triangular array of phrases for this source sentence
    *                the type PhraseInfoPtr is never used: this method only
    *                ever tests if a subvector within phrases is empty or not.
    * @return true iff this configuration is either monotonic or a phrase swap.
    */
   template<class PhraseInfoPtr>
   static bool isPhraseSwap(const UintSet& cov, Range last_phrase,
         Range new_phrase, Uint sourceLength, vector<PhraseInfoPtr> **phrases)
   {
      // Case 0: if cov has more than 2 contiguous sub ranges (and therefore 2
      // or more holes), this cannot be a simple phrase swap.
      if ( cov.size() > 2 )
         return false;

      // Case 1: cov is one block to the end of the sentence
      else if ( cov.size() == 1 && cov[0].end == sourceLength ) {
         // Case 1a: new_phrase starts at the beginning of cov - this is the
         // monotonic case and thus always allowed
         if ( new_phrase.start == cov[0].start )
            return true;
         // Case 1b: new_phrase is further down, and there exists a phrase that
         // can cover all non-covered words before new_phrase
         assert(new_phrase.start > cov[0].start);
         assert(new_phrase.start < sourceLength);
         return ! phrases[cov[0].start][new_phrase.start-cov[0].start-1]
                  .empty();
      }

      // Case 2: cov is two blocks or one block not extending to the end of the
      // sentence
      else {
         assert(cov.size() <= 2);
         // Case 2a: new_phrase is continguous to and preceedes last_phrase,
         // and the result of adding new_phrase will leave no holes.
         if ( new_phrase == cov[0] && 
              last_phrase.start == cov[0].end &&
              last_phrase.end == (cov.size()==2 ? cov[1].start : sourceLength) )
            return true;
         else
            return false;
      }
   }

protected:

   /// Implements respectsDistLimit() when distLimitExt=false.
   static bool respectsDistLimitCons(const UintSet& cov,
         Range last_phrase, Range new_phrase, int distLimit, Uint sourceLength,
         const UintSet* resulting_cov);

   /// Implements respectsDistLimit() when distLimitExt=true.
   static bool respectsDistLimitExt(const UintSet& cov,
         Range last_phrase, Range new_phrase, int distLimit, Uint sourceLength,
         const UintSet* resulting_cov);

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

   virtual double partialScore(const PartialTranslation& trans);

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

/**
 * Phrase displacement distortion model: simply count non-contiguous phrases.
 * Each time a phrase does not immediately follow the previous one, -1 is added
 * to the score.  Motivation: with the -dist-phrase-swap option, full phrase
 * swaps are allowed, but incur a severe distortion penalty for longer phrases.
 * In the phrase-swapping paradigm, however, we want to consider swapping
 * phrases to have a fixed cost that doesn't depend on the phrase length.
 * That's what this feature provides.  We can use both WordDisplacement and
 * PhraseDisplacement together, and let cow determine their respective weights.
 */
class PhraseDisplacement : public WordDisplacement
{
public:
   virtual double score(const PartialTranslation& pt);

   virtual double futureScore(const PartialTranslation &trans);

   // same implementation as WordDisplacement
   //virtual double partialScore(const PartialTranslation& trans);
   //virtual Uint computeRecombHash(const PartialTranslation &pt);
   //virtual bool isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2);
   //virtual double precomputeFutureScore(const PhraseInfo& phrase_info);

};

}

#endif
