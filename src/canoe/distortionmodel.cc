/**
 * @author George Foster
 * @file distortionmodel.cc  Implementation of all related distortion model decoder feature.
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <errors.h>
#include "distortionmodel.h"
#include <str_utils.h>
#include <iostream>

using namespace Portage;

/************************** DistortionModel **********************************/

DistortionModel* DistortionModel::create(const string& name_and_arg, bool fail)
{
   DistortionModel* m = NULL;

   // Separate the model name and argument, introduced by # if present.
   vector<string> arg_split;
   split(name_and_arg, arg_split, "#", 2);
   string name(arg_split.empty() ? "" : arg_split[0]);
   string arg(arg_split.size() < 2 ? "" : arg_split[1]);

   if (name == "WordDisplacement") {
      m = new WordDisplacement();
   } else if (name == "ZeroInfo") {
      m = new ZeroInfoDistortion();
   } else if (name == "PhraseDisplacement") {
      m = new PhraseDisplacement();
   } else if (fail) {
      error(ETFatal, "unknown distortion model: " + name);
   }

   return m;
}

bool DistortionModel::respectsDistLimitCons(const UintSet& cov,
      Range last_phrase, Range new_phrase, int distLimit, Uint sourceLength,
      const UintSet* resulting_cov)
{
   if ( distLimit == NO_MAX_DISTORTION ) return true;

   // distortion limit test 1: start of new phrase can't be too far from end of
   // last phrase
   if ( abs(int(last_phrase.end) - int(new_phrase.start)) > distLimit )
      return false;

   // distortion limit test 2: end of new phrase can't be too far from first
   // non-covered word - this is a conservative version as describe by Aaron's
   // original comment regarding his implementation of this test in
   // BasicModel::computeFutureScore():
   // To prevent distortion limit violations later, it is checked that the
   // distance from the current position to the first untranslated word is at
   // most the distortion limit and that the distance between ranges of
   // untranslated words is at most the distortion limit.  This is not an iff
   // condition - I am being more conservative and penalizing some partial
   // translations for which it is possible to complete without violating the
   // distortion limit.  I am confident however that there is an iff condition
   // that could be checked in O(trans.sourceWordsNotCovered.size()).
   assert (!cov.empty());
   if ( new_phrase.start != cov.front().start &&
        abs(int(new_phrase.end) - int(cov.front().start)) > distLimit )
      return false;

   // After this strict conservative test, we shouldn't need to check that
   // covered blocks are smaller than distLimit, since they can't even end
   // further away than distLimit from cov.front().start!  However, if this
   // test is combined with the phrase swapping test in an OR statement, we
   // still need to validate the jumps.

   UintSet next_cov;
   if ( resulting_cov == NULL ) {
      subRange(next_cov, cov, new_phrase);
      resulting_cov = &next_cov;
   }
   // Complete coverage is necessarily OK.
   if ( resulting_cov->empty() )
      return true;

   // Final case: make sure we can jump through all the covered blocks.
   int last_end = new_phrase.end;
   for ( Uint i(0), end(resulting_cov->size()); i < end; ++i ) {
      if ( abs(int((*resulting_cov)[i].start) - last_end) > distLimit )
         return false;
      last_end = (*resulting_cov)[i].end;
   }
   if ( int(sourceLength) - last_end > distLimit )
      return false;

   return true;
}

bool DistortionModel::respectsDistLimitExt(const UintSet& cov,
      Range last_phrase, Range new_phrase, int distLimit, Uint sourceLength,
      const UintSet* resulting_cov)
{
   if ( distLimit == NO_MAX_DISTORTION ) return true;

   // distortion limit test 1: start of new phrase can't be too far from end of
   // last phrase
   if ( abs(int(last_phrase.end) - int(new_phrase.start)) > distLimit )
      return false;

   // distortion limit test 2: start of new phrase can't be too far from first
   // non-covered word
   assert (!cov.empty());
   if ( new_phrase.start > cov.front().start + distLimit )
      return false;

   // simple case: it's always OK to cover the first words in cov
   if ( new_phrase.start == cov.front().start )
      return true;

   // distortion limit test 3: when a new phrase starts within the limit but
   // ends past cov.front().start + distLimit, its end must be such that it's
   // still possible to finish covering cov without violating distLimit in
   // subsequent steps.  At least one successfull path must exist: To finish
   // covering cov, we need two non-covered positions within 1 + the distortion
   // limit from the end of the new phrase.
   if ( new_phrase.end > cov.front().start + distLimit ) {
      // case 3a: new phrase is too long, so it's not possible to have 2
      // non-covered postions in the jump back space
      if ( new_phrase.end - new_phrase.start + 1 > Uint(distLimit) )
         return false;
   }

   // resulting_cov is needed both for case 3b and the final case below, so
   // factor it out of the main case 3 if statement.
   UintSet next_cov;
   if ( resulting_cov == NULL ) {
      subRange(next_cov, cov, new_phrase);
      resulting_cov = &next_cov;
   }
   // Complete coverage is necessarily OK.
   if ( resulting_cov->empty() )
      return true;

   // This final case is also cheaper to calculate than case 3b, so do it first
   // Final case: make sure we can jump forward through all the covered blocks.
   Uint last_end = resulting_cov->front().end;
   for ( Uint i(1), end(resulting_cov->size()); i < end; ++i ) {
      if ( (*resulting_cov)[i].start > last_end + distLimit )
         return false;
      last_end = (*resulting_cov)[i].end;
   }

   // And now let's get back and finish distortion limit test 3:
   if ( new_phrase.end > cov.front().start + distLimit ) {
      // Case 3b: we need to actually count the number of free positions in the
      // jump back space.
      UintSet jump_back_space;
      assert(int(new_phrase.end)-distLimit-1 >= 0);
      intersectRange(jump_back_space, *resulting_cov,
                     Range(new_phrase.end-distLimit-1, new_phrase.start));
      if ( jump_back_space.empty() ||
           jump_back_space.size() == 1 && 
              jump_back_space.front().end - jump_back_space.front().start < 2 )
         return false;
   }
   if ( int(sourceLength) - int(last_end) > distLimit )
      return false;

   // Whew, we passed all tests.  Yay!
   return true;
}

/************************** WordDisplacement *********************************/

double WordDisplacement::score(const PartialTranslation& trans)
{
   double result = -(double)
      abs((int)trans.lastPhrase->src_words.start -
          (int)trans.back->lastPhrase->src_words.end);

   // Add distortion cost to end of sentence
   // Note: Other decoders don't do this (I think) but I decided to for
   // symmetry, as this is done for the first word in the sentence.
   // eg. 0 1 2 -> 0 2 1 and 0 1 2 -> 1 0 2 should have the same distortion
   // cost, which it doesn't the way other decoders do things.
   if (trans.sourceWordsNotCovered.empty())
      {
         assert(trans.lastPhrase->src_words.end <= sentLength);
         result -= (double)(sentLength - trans.lastPhrase->src_words.end);
      } // if

   return result;

}

// Note: PhraseDisplacement relies on this implementation of partialScore(),
// so if any changed is made here, class PhraseDisplacement will need to
// overridden partialScore().
double WordDisplacement::partialScore(const PartialTranslation& trans)
{
   // score() only uses the source range of the last phrase, so it does what
   // this function should do.
   return score(trans);
}


// Technically, this should capture the end of the source phrase aligned with
// the rightmost target phrase, but it doesn't have to be perfct, so I'm going
// with the way Aaron originally defined it.
// EJJ Nov 2007: it's trivial to do a bit better, so why not!?  I removed
// "return 0" for the hash, and used pt.lastPhrase->src_words.end instead.

Uint WordDisplacement::computeRecombHash(const PartialTranslation &pt)
{
   //return 0;
   return pt.lastPhrase->src_words.end;
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
      if (it->end > sentLength) cerr << it->end << " " << sentLength << endl;
      assert(it->end <= sentLength);
      distScore -= abs((int)lastEnd - (int)it->start);
      lastEnd = it->end;
   } // for
   distScore -= (sentLength - lastEnd);

   return distScore;
}



/************************** PhraseDisplacement ******************************/

double PhraseDisplacement::score(const PartialTranslation& trans)
{
   double result = 0.0;
   if ( trans.lastPhrase->src_words.start !=
        trans.back->lastPhrase->src_words.end )
      result += -1.0;

   // Add distortion cost to end of sentence
   if (trans.sourceWordsNotCovered.empty())
   {
      assert(trans.lastPhrase->src_words.end <= sentLength);
      if ( trans.lastPhrase->src_words.end != sentLength )
         result += -1.0;
   }

   return result;
}

double PhraseDisplacement::futureScore(const PartialTranslation &trans)
{
   double distScore = 0;
   Uint lastEnd = trans.lastPhrase->src_words.end;
   for (UintSet::const_iterator it = trans.sourceWordsNotCovered.begin();
        it != trans.sourceWordsNotCovered.end(); it++)
   {
      assert(it->start < it->end);
      if (it->end > sentLength) cerr << it->end << " " << sentLength << endl;
      assert(it->end <= sentLength);
      distScore += ( (lastEnd != it->start) ? -1.0 : 0.0 );
      lastEnd = it->end;
   } // for
   distScore += ( (sentLength != lastEnd) ? -1.0 : 0.0 );

   return distScore;
}

