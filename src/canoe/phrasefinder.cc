/**
 * @author Aaron Tikuisis
 * @file phrasefinder.cc  This file contains an implementation of
 * RangePhraseFinder, which is an abstraction of the method used to find the
 * set of phrases which can be added to a partial translation.
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
#include "basicmodel.h"

#include <iostream>

using namespace Portage;

RangePhraseFinder::RangePhraseFinder(vector<PhraseInfo *> **phrases, BasicModel& model)
: model(&model)
, phrases(phrases)
, sentLength(model.getSourceLength())
, distLimit(model.c->distLimit)
, itgLimit(model.c->itgLimit)
, distLimitSimple(model.c->distLimitSimple)
, distLimitExt(model.c->distLimitExt)
, distPhraseSwap(model.c->distPhraseSwap)
, distLimitITG(model.c->distLimitITG)
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
      if ((*it)->empty()) continue;

      // Early detection of filter-feature constraint violations
      if (model->earlyFilterFeatureViolation(t, (*it)->front()->src_words))
         continue;

      // Early detection of ITG constraint temporarily here, until it gets
      // turned into a filter feature.
      if (distLimitITG &&
          !DistortionModel::respectsITG(itgLimit, t.shiftReduce, (*it)->front()->src_words))
         continue;

      // Do the distortion limit tests outside the jt loop, since we know that
      // all phrases in a given "pick" share the same source range.
      if (!model->respectsDistortionLimit(t, (*it)->front()->src_words))
         continue;

      // All filters passed, keep these phrases as candidates.
      for (vector<PhraseInfo *>::const_iterator jt = (*it)->begin(); jt < (*it)->end(); ++jt)
         p.push_back(*jt);
   }
} // findPhrases

