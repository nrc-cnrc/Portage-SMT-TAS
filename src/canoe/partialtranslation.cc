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
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
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
PartialTranslation::PartialTranslation(): back(NULL) {}

/*
 * Puts the last num words of the target partial-sentence into the vector words (which
 * should be empty).  The front of the vector will contain the leftmost word.  If the
 * partial-sentence contains fewer than num words, all the words will be entered into the
 * vector.
 * @param words	The vector to be filled with the last num target words in the
 * 		translation.
 * @param num	The number of words to get.
 */
void PartialTranslation::getLastWords(Phrase &words, Uint num) const
{
    // Preallocate enough memory in the words vector, if necessary
    if ( num + words.size() > words.capacity() )
    {
        words.reserve( num + words.size() );
    }

    _getLastWords(words, num);
} // getLastWords

void PartialTranslation::_getLastWords(Phrase &words, Uint num) const
{
    if (lastPhrase != NULL)
    {
	if (num > lastPhrase->phrase.size() && back != NULL)
	{
	    // Get tail of previous partial translation
	    back->_getLastWords(words, num - lastPhrase->phrase.size());
	} // if

	// Copy over part or all of the entire last phrase
	for (Uint i = max(0, lastPhrase->phrase.size() - num);
             i < lastPhrase->phrase.size();
             i++)
	{
            //assert(words.capacity() > words.size());
	    words.push_back(lastPhrase->phrase[i]);
	} // for
    } // if
} // _getLastWords

/**
 * Compares the num last words of this with that
 * @param that      The other partial translation for the comparison
 * @param num       The number of words to compare
 * @return          true iff the n last words of this and that and the same
 *
 * EJJ 06Sept2005 This is a messy non-recursive implementation, but much faster
 * because it doesn't need to copy the last words.  BasicModel::isRecombinable
 * is called very often, and does equality on the result of getLastWords on the
 * two partial translation is expensive because it requires lots of memory
 * allocation for building this last phrases.  Here we just compare the two end
 * phrases directly in the linked list structure that a PartialTranslation is.
 */
bool PartialTranslation::sameLastWords(const PartialTranslation &that, Uint num, Uint verbosity) const
{
    const PartialTranslation *thisTrans = this;  // Current link in this linked list
    const PartialTranslation *thatTrans = &that; // Current link in that linked list
    Uint thisPos = 0;  // number of visited tokens in this link's last Phrase
    Uint thatPos = 0;  // number of visited tokens in that link's last Phrase
    if ( verbosity > 3 ) {
        Phrase endPhrase;
        getLastWords(endPhrase, num);
        cerr << "PT:sameLastWords this.getLastWords:";
        for ( Uint i = 0 ; i < endPhrase.size(); i++ ) {
            cerr << " " << endPhrase[i];
        }
        cerr << endl;
        endPhrase.clear();
        that.getLastWords(endPhrase, num);
        cerr << "PT:sameLastWords that.getLastWords:";
        for ( Uint i = 0 ; i < endPhrase.size(); i++ ) {
            cerr << " " << endPhrase[i];
        }
        cerr << endl;
    }

    while ( num > 0 ) {
        if ( verbosity > 3 )
            cerr << "PT::sameLastWords:a"
                 << " thisTrans: " << thisTrans
                 << " thatTrans: " << thatTrans
                 << " thisPos: " << thisPos
                 << " thatPos: " << thatPos
                 << " num: " << num
                 << endl;
        bool thisWordExists = true, thatWordExists = true;

        // Go to the next word in thisTrans, possibly in the current thisTrans
        // object, or following the back link if necessary.
        while ( (thisTrans->lastPhrase != NULL) && 
                (thisTrans->lastPhrase->phrase.size() == thisPos) ) {
            if ( thisTrans->back != NULL ) {
                thisTrans = thisTrans->back;
                thisPos = 0;
            } else {
                thisWordExists = false;
                break;
            }
        }
        if ( thisTrans->lastPhrase == NULL ) thisWordExists = false;

        // Go to the next word in thatTrans
        while ( (thatTrans->lastPhrase != NULL) && 
                (thatTrans->lastPhrase->phrase.size() == thatPos) ) {
            if ( thatTrans->back != NULL ) {
                thatTrans = thatTrans->back;
                thatPos = 0;
            } else {
                thatWordExists = false;
                break;
            }
        }
        if ( thatTrans->lastPhrase == NULL ) thatWordExists = false;

        // If either translation has exhausted all its words, we're done.
        if ( !thisWordExists || !thatWordExists )
            return thisWordExists == thatWordExists;

        assert ( thisTrans->lastPhrase );
        assert ( thatTrans->lastPhrase );
        assert ( thisTrans->lastPhrase->phrase.size() > thisPos );
        assert ( thatTrans->lastPhrase->phrase.size() > thatPos );

        // Compare the next words themselves.
        Uint thisWord = thisTrans->lastPhrase->phrase[
            thisTrans->lastPhrase->phrase.size() - thisPos - 1];
        Uint thatWord = thatTrans->lastPhrase->phrase[
            thatTrans->lastPhrase->phrase.size() - thatPos - 1];

        if ( verbosity > 3 ) {
            cerr << "PT::sameLastWords:b"
                 << " thisTrans: " << thisTrans
                 << " thatTrans: " << thatTrans
                 << " thisPos: " << thisPos
                 << " thatPos: " << thatPos
                 << " thisSize: " << thisTrans->lastPhrase->phrase.size()
                 << " thatSize: " << thatTrans->lastPhrase->phrase.size()
                 << " thisWord: " << thisWord
                 << " thatWord: " << thatWord
                 << endl;
            cerr << " thisTrans->lastPhrase->phrase:";
            for (Uint i = 0; i < thisTrans->lastPhrase->phrase.size(); i++) {
                cerr << " " << thisTrans->lastPhrase->phrase[i];
            }
            cerr << endl;
            cerr << " thatTrans->lastPhrase->phrase:";
            for (Uint i = 0; i < thatTrans->lastPhrase->phrase.size(); i++) {
                cerr << " " << thatTrans->lastPhrase->phrase[i];
            }
            cerr << endl;
        }


        if ( thisWord != thatWord ) return false;
        thisPos++;
        thatPos++;
        num--;
    } // while
    
    // if this while loop exits, all num words were the same.
    return true;
} // sameLastWords

/**
 * Puts all the words of the target partial-sentence into the vector words (which
 * should be empty).  The front of the vector will contain the first word in the
 * partial-sentence.
 * @param words	The vector to be filled with the words in the translation.
 */
void PartialTranslation::getEntirePhrase(Phrase &words) const
{
    getLastWords(words, getLength());
} // getEntirePhrase

/**
 * Determines the target length of this partial translation.
 * @return	The length of the target partial-sentence.
 */
Uint PartialTranslation::getLength() const
{
    if (lastPhrase != NULL && back != NULL)
    {
	return lastPhrase->phrase.size() + back->getLength();
    } else if (lastPhrase != NULL)
    {
	return lastPhrase->phrase.size();
    } else
    {
	return 0;
    } // if
} // if
