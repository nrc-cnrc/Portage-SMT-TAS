/**
 * @author Aaron Tikuisis
 * @file partialtranslation.cc  This file contains the implementation of the
 * PartialTranslation object, representing a translation prefix, not
 * necessarily covering all source words.
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

const PhraseInfo PartialTranslation::EmptyPhraseInfo;

void PartialTranslation::setLmContextSize(Uint size) const
{
   if (size >= ArrayUint4::MAX)
      error(ETFatal, "Trying to set lm context size to unsupported value %u. When using -minimize-lm-context-size, you may not use n-gram LMs of order higher than %u.", size, ArrayUint4::MAX);
   contextSizes.set(0, size);
}

void PartialTranslation::setBiLMContextSize(Uint biLM_ID, Uint size) const
{
   if (size >= ArrayUint4::MAX)
      error(ETFatal, "Trying to set BiLM context size to unsupported value %u. When using -minimize-lm-context-size, you may not use n-gram LMs of order higher than %u.", size, ArrayUint4::MAX);
   contextSizes.set(biLM_ID, size);
}

void PartialTranslation::setAllContextSizes(Uint size) const
{
   if (size >= ArrayUint4::MAX)
      error(ETFatal, "Trying to set lm and bilm context sizes to unsupported value %u. When using -minimize-lm-context-size, you may not use n-gram LMs of order higher than %u.", size, ArrayUint4::MAX);
   contextSizes = ArrayUint4(size);
}

// CAC: It breaks my heart, but this needs to stay to enable
// legacy testing.
PartialTranslation::PartialTranslation(PhraseInfo* phrase)
   : back(NULL)
   , lastPhrase(phrase)
   , numSourceWordsCovered(0)
   , contextSizes(-1) // == all uninit
   , levInfo(NULL)
   , shiftReduce(NULL)
{}

PartialTranslation::PartialTranslation(Uint sourceLen,
                                       bool usingLev,
                                       bool usingSR)
   : back(NULL)
   , lastPhrase(&EmptyPhraseInfo)
   , numSourceWordsCovered(0)
   , contextSizes(1) // the initial empty state always provides <s> (or its bitoken) as context
   , levInfo(usingLev ? new PartialTranslation::levenshteinInfo() : NULL)
   , shiftReduce(usingSR ? new ShiftReducer(sourceLen) : NULL)
{
   // Set the range of words not covered to be the full range of words
   if ( sourceLen > 0 ) {
      Range fullRange(0, sourceLen);
      sourceWordsNotCovered.push_back(fullRange);
   }

   // Initialize the empty Levenshtein info
   if (usingLev)
      levInfo->levDistance = 0;

   /*static bool hasBeen = false;
   if (!hasBeen) {
      hasBeen = true;
      if (levInfo)
         cerr << "Using lev info" << endl;
      else   
         cerr << "NOT Using lev info" << endl;
   }*/
}

PartialTranslation::PartialTranslation(const PartialTranslation* trans0,
      const PhraseInfo* phrase, const UintSet* preCalcSourceWordsCovered)
   : back(trans0)
   , lastPhrase(phrase)
   , contextSizes(-1) // == all uninit
   , levInfo(trans0->levInfo ? new PartialTranslation::levenshteinInfo() : NULL)
   , shiftReduce(trans0->shiftReduce
                 ? new ShiftReducer(phrase->src_words,trans0->shiftReduce)
                 : NULL)
{
   assert(trans0 != NULL);
   assert(phrase != NULL);

   // Compute foreign words covered
   const Range &newWords = phrase->src_words;
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
   if (shiftReduce) delete shiftReduce, shiftReduce = NULL;
}

void PartialTranslation::getLastWords(VectorPhrase &words, Uint num, bool backward)
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

void PartialTranslation::_getLastWords(VectorPhrase &words, Uint num) const
{
   if (lastPhrase != NULL) {
      const Uint last_phrase_size(getPhrase().size());
      if (num > last_phrase_size && back != NULL) {
         // Get tail of previous partial translation
         back->_getLastWords(words, num - last_phrase_size);
      }

      // Copy over part or all of the entire last phrase
      Phrase::const_iterator w_it(getPhrase().begin());
      if ( num < last_phrase_size )
         w_it += last_phrase_size - num;
      Phrase::const_iterator w_end(getPhrase().end());
      for ( ; w_it != w_end; ++w_it )
         words.push_back(*w_it);
   }
} // _getLastWords

void PartialTranslation::_getLastWordsBackward(VectorPhrase &words, Uint num) const
{
   const PartialTranslation* pt = this;
   while ( pt != NULL && pt->lastPhrase != NULL && num > 0 ) {
      Phrase::const_reverse_iterator w_it(pt->getPhrase().rbegin());
      Phrase::const_reverse_iterator w_end(pt->getPhrase().rend());
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
      thisPos = thisTrans->getPhrase().rbegin();
      thisEnd = thisTrans->getPhrase().rend();
      thisWordExists = true;
   }
   if ( thatTrans && thatTrans->lastPhrase ) {
      thatPos = thatTrans->getPhrase().rbegin();
      thatEnd = thatTrans->getPhrase().rend();
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
               thisPos = thisTrans->getPhrase().rbegin();
               thisEnd = thisTrans->getPhrase().rend();
               thisWordExists = true;
            }
         }
      }
      while ( thatWordExists && thatPos == thatEnd ) {
         thatWordExists = false;
         if ( thatTrans->back ) {
            thatTrans = thatTrans->back;
            if ( thatTrans && thatTrans->lastPhrase ) {
               thatPos = thatTrans->getPhrase().rbegin();
               thatEnd = thatTrans->getPhrase().rend();
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

void PartialTranslation::getEntirePhrase(VectorPhrase &words) const
{
   getLastWords(words, getLength());
} // getEntirePhrase

Uint PartialTranslation::getLength() const
{
   return getPhrase().size() + (back ? back->getLength() : 0);
} // getLength
