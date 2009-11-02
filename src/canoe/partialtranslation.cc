/**
 * @author Aaron Tikuisis
 * @file partialtranslation.cc  This file contains the implementation of the
 * PartialTranslation object, representing a translation prefix, not
 * necessarily covering all source words.
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

#include "phrasedecoder_model.h"
#include "canoe_general.h"
#include <vector>
#include <iostream>

using namespace std;
using namespace Portage;

/*
 * Creates a new partial translation object.
 */
PartialTranslation::PartialTranslation(bool usingLev)
   : back(NULL)
   , lastPhrase(NULL)
   , levInfo(usingLev ? new PartialTranslation::levenshteinInfo() : NULL)
{ 
   /*static bool hasBeen = false;
   if (!hasBeen) {
      hasBeen = true;
      if (levInfo)
         cerr << "Using lev info" << endl;
      else   
         cerr << "NOT Using lev info" << endl;
   }*/
}

PartialTranslation::PartialTranslation(PartialTranslation* trans0,
      PhraseInfo* phrase, const UintSet* preCalcSourceWordsCovered)
   : back(trans0)
   , lastPhrase(phrase)
   , levInfo(trans0->levInfo ? new PartialTranslation::levenshteinInfo() : NULL)
{
   assert(trans0 != NULL);
   assert(phrase != NULL);

   // Compute foreign words covered
   Range &newWords = phrase->src_words;
   assert(trans0->numSourceWordsCovered + newWords.end - newWords.start > 0);
   numSourceWordsCovered = trans0->numSourceWordsCovered +
      newWords.end - newWords.start;

   if ( preCalcSourceWordsCovered ) 
      sourceWordsNotCovered = *preCalcSourceWordsCovered;
   else
      subRange(sourceWordsNotCovered, trans0->sourceWordsNotCovered, newWords);
}

PartialTranslation::~PartialTranslation()
{
   if (levInfo) delete levInfo, levInfo = NULL;
}

void PartialTranslation::getLastWords(Phrase &words, Uint num, bool backward)
   const
{
   // Preallocate enough memory in the words vector, if necessary
   if ( num + words.size() > words.capacity() ) {
      words.reserve( num + words.size() );
   }

   if ( backward )
      _getLastWordsBackward(words, num);
   else
      _getLastWords(words, num);
} // getLastWords

void PartialTranslation::_getLastWords(Phrase &words, Uint num) const
{
   if (lastPhrase != NULL) {
      const Uint last_phrase_size(lastPhrase->phrase.size());
      if (num > last_phrase_size && back != NULL) {
         // Get tail of previous partial translation
         back->_getLastWords(words, num - last_phrase_size);
      }

      // Copy over part or all of the entire last phrase
      Phrase::const_iterator w_it(lastPhrase->phrase.begin());
      if ( num < last_phrase_size )
         w_it += last_phrase_size - num;
      Phrase::const_iterator w_end(lastPhrase->phrase.end());
      for ( ; w_it != w_end; ++w_it )
         words.push_back(*w_it);
   }
} // _getLastWords

void PartialTranslation::_getLastWordsBackward(Phrase &words, Uint num) const
{
   const PartialTranslation* pt = this;
   PhraseInfo* lp = NULL;
   while ( pt != NULL && (lp = pt->lastPhrase) && num > 0 ) {
      Phrase::const_reverse_iterator w_it(lp->phrase.rbegin());
      Phrase::const_reverse_iterator w_end(lp->phrase.rend());
      while ( num > 0 && w_it != w_end ) {
         words.push_back(*w_it);
         ++w_it;
         --num;
      }

      // Next PartialTranslation
      pt = pt->back;
   }
}

/*
 * EJJ 06Sept2005 This is a messy non-recursive implementation, but much faster
 * because it doesn't need to copy the last words.  BasicModel::isRecombinable
 * is called very often, and does equality on the result of getLastWords on the
 * two partial translation is expensive because it requires lots of memory
 * allocation for building this last phrases.  Here we just compare the two end
 * phrases directly in the linked list structure that a PartialTranslation is.
 */
bool PartialTranslation::sameLastWords(const PartialTranslation &that,
                                       Uint num, Uint verbosity) const
{
   // Current link in this/that linked list
   const PartialTranslation *thisTrans = this;
   const PartialTranslation *thatTrans = &that;
   // Current position in this/that linked list's last Phrase
   Phrase::const_reverse_iterator thisPos, thatPos, thisEnd, thatEnd;
   bool thisWordExists(false), thatWordExists(false);
   if ( thisTrans && thisTrans->lastPhrase ) {
      thisPos = thisTrans->lastPhrase->phrase.rbegin();
      thisEnd = thisTrans->lastPhrase->phrase.rend();
      thisWordExists = true;
   }
   if ( thatTrans && thatTrans->lastPhrase ) {
      thatPos = thatTrans->lastPhrase->phrase.rbegin();
      thatEnd = thatTrans->lastPhrase->phrase.rend();
      thatWordExists = true;
   }
   while ( num > 0 ) {
      // If at the end for either this or that phrase, go down the linked
      // list to the continuation.
      while ( thisWordExists && thisPos == thisEnd ) {
         thisWordExists = false;
         if ( thisTrans->back ) {
            thisTrans = thisTrans->back;
            if ( thisTrans && thisTrans->lastPhrase ) {
               thisPos = thisTrans->lastPhrase->phrase.rbegin();
               thisEnd = thisTrans->lastPhrase->phrase.rend();
               thisWordExists = true;
            }
         }
      }
      while ( thatWordExists && thatPos == thatEnd ) {
         thatWordExists = false;
         if ( thatTrans->back ) {
            thatTrans = thatTrans->back;
            if ( thatTrans && thatTrans->lastPhrase ) {
               thatPos = thatTrans->lastPhrase->phrase.rbegin();
               thatEnd = thatTrans->lastPhrase->phrase.rend();
               thatWordExists = true;
            }
         }
      }

      // If either translation has exhausted all its words, we're done.
      if ( !thisWordExists || !thatWordExists )
         return thisWordExists == thatWordExists;

      // Compare the next words themselves
      if ( *thisPos != *thatPos ) return false;

      ++thisPos;
      ++thatPos;
      --num;
   }

   // if the while loop above exits, all num words were the same.
   return true;
} // sameLastWords

void PartialTranslation::getEntirePhrase(Phrase &words) const
{
   getLastWords(words, getLength());
} // getEntirePhrase

Uint PartialTranslation::getLength() const
{
   if (lastPhrase != NULL && back != NULL) {
      return lastPhrase->phrase.size() + back->getLength();
   } else if (lastPhrase != NULL) {
      return lastPhrase->phrase.size();
   } else {
      return 0;
   }
} // getLength
