/**
 * @author Aaron Tikuisis
 * @file phrasefinder.cc  This file contains an implementation of
 * RangePhraseFinder, which is an abstraction of the method used to find the
 * set of phrases which can be added to a partial translation.
 * 
 * $Id$
 * 
 * Canoe Decoder
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "phrasefinder.h"
#include "canoe_general.h"
#include "phrasedecoder_model.h"
#include "distortionmodel.h"

#include <iostream>

using namespace Portage;

RangePhraseFinder::RangePhraseFinder(vector<PhraseInfo *> **phrases,
   Uint sentLength,
   int	distLimit,
   int  itgLimit,
   bool distLimitSimple,
   bool distLimitExt,
   bool distPhraseSwap,
   bool distLimitITG)
: phrases(phrases)
, sentLength(sentLength)
, distLimit(distLimit)
, itgLimit(itgLimit)
, distLimitSimple(distLimitSimple)
, distLimitExt(distLimitExt)
, distPhraseSwap(distPhraseSwap)
, distLimitITG(distLimitITG)
{
   assert(distLimit >= 0 || distLimit == NO_MAX_DISTORTION);
}

void RangePhraseFinder::findPhrases(vector<PhraseInfo *> &p, PartialTranslation &t)
{
   UintSet eSet;
   if (distLimit != NO_MAX_DISTORTION && !distPhraseSwap && !distLimitSimple)
   {
      Range limit(max(0, int(t.lastPhrase->src_words.end) - distLimit),
                  sentLength);
      intersectRange(eSet, t.sourceWordsNotCovered, limit);
   } // if
   UintSet &set((distLimit != NO_MAX_DISTORTION && !distPhraseSwap && !distLimitSimple)
                 ? eSet : t.sourceWordsNotCovered);
   if ( set.empty() ) return;

   vector<const vector<PhraseInfo *>*> picks;
   pickItemsByRange(picks, phrases, set);

   // EJJ Count how many PhraseInfo's we might keep, so that we can
   // pre-allocate the memory for p, the result vector
   Uint phraseCount = 0;
   for (vector< const vector<PhraseInfo *>*>::const_iterator it = picks.begin(); 
         it < picks.end(); it++)
      phraseCount += (*it)->size();
   p.reserve(phraseCount);

   // Put all the PhraseInfo's into a single vector, results
   for ( vector< const vector<PhraseInfo *>*>::const_iterator it = picks.begin();
         it < picks.end(); ++it)
   {
      // Do the distortion limit tests outside the jt loop, since we know that
      // all phrases in a given "pick" share the same source range.
      if ( ! (*it)->empty() &&
           (
            (
             (!distLimitITG
              || DistortionModel::respectsITG(itgLimit,
                                              t.shiftReduce,
                                              (*it)->front()->src_words))
             &&
             DistortionModel::respectsDistLimit(t.sourceWordsNotCovered,
                 t.lastPhrase->src_words, (*it)->front()->src_words, distLimit,
                 sentLength, distLimitSimple, distLimitExt)
             )
            ||
              (distPhraseSwap && DistortionModel::isPhraseSwap(
                 t.sourceWordsNotCovered, t.lastPhrase->src_words,
                 (*it)->front()->src_words, sentLength, phrases))
           )
         )
      {
         for ( vector<PhraseInfo *>::const_iterator jt = (*it)->begin();
               jt < (*it)->end(); ++jt)
         {
            p.push_back(*jt);
         }
      }
   }
} // findPhrases

