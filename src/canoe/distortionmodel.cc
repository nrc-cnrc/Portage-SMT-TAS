/**
 * @author George Foster
 * @file distortionmodel.cc  Implementation of all related distortion model decoder feature.
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada /
 * Copyright 2005, National Research Council of Canada
 */

#include <errors.h>
#include "distortionmodel.h"
#include <str_utils.h>
#include <iostream>

using namespace Portage;

/************************** DistortionModel **********************************/

DistortionModel* DistortionModel::create(const string& name, const string& arg, bool fail)
{
   DistortionModel* m = NULL;

   if (name == "WordDisplacement") {
      m = new WordDisplacement();
   } else if (name == "ZeroInfo") {
      m = new ZeroInfoDistortion();
   } else if (name == "none") {
      m = 0;
   } else if (fail) {
      error(ETFatal, "unknown distortion model: " + name);
   }

   return m;
}

/************************** WordDisplacement *********************************/

double WordDisplacement::score(const PartialTranslation& trans)
{
   double result = -(double)
      abs((int)trans.lastPhrase->src_words.start -
          (int)trans.back->lastPhrase->src_words.end);

   // Add distortion cost to end of sentence
   // Note: Other decoders don't do this (I think) but I decided to for symmetry, as this is
   // done for the first word in the sentence.
   // eg. 0 1 2 -> 0 2 1 and 0 1 2 -> 1 0 2 should have the same distortion cost, which
   // it doesn't the way other decoders do things.
   if (trans.sourceWordsNotCovered.size() == 0)
      {
         assert(trans.lastPhrase->src_words.end <= sentLength);
         result -= (double)(sentLength - trans.lastPhrase->src_words.end);
      } // if

   return result;

}

// Technically, this should capture the end of the source phrase aligned with
// the rightmost target phrase, but it doesn't have to be perfct, so I'm going
// with the way Aaron originally defined it.

Uint WordDisplacement::computeRecombHash(const PartialTranslation &pt)
{
   return 0;
}

bool WordDisplacement::isRecombinable(const PartialTranslation &pt1, const PartialTranslation &pt2)
{
   return pt1.lastPhrase->src_words.end == pt2.lastPhrase->src_words.end;
}

double WordDisplacement::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   return 0;
}

double WordDisplacement::futureScore(const PartialTranslation &trans)
{
   double distScore = 0;
   Uint lastEnd = trans.lastPhrase->src_words.end;
   for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin();
        it != trans.sourceWordsNotCovered.end(); it++)
   {
      assert(it->start < it->end);
      assert(it->end <= sentLength);
      distScore -= abs((int)lastEnd - (int)it->start);
      lastEnd = it->end;
   } // for
   distScore -= (sentLength - lastEnd);

   return distScore;
}

