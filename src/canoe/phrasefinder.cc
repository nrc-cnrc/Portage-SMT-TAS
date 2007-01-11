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
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada
 */

#include "phrasefinder.h"
#include "canoe_general.h"
#include "phrasedecoder_model.h"

#include <iostream>

using namespace Portage;

RangePhraseFinder::RangePhraseFinder(vector<PhraseInfo *> **phrases,
   Uint sentLength,
   int	maxDistortion)
: phrases(phrases)
, sentLength(sentLength)
, maxDistortion(maxDistortion)
{
    assert(maxDistortion >= 0 || maxDistortion == NO_MAX_DISTORTION);
}

void RangePhraseFinder::findPhrases(vector<PhraseInfo *> &p, PartialTranslation &t)
{
    UintSet eSet;
    if (maxDistortion != NO_MAX_DISTORTION)
    {
	Range limit(max(0, (int)t.lastPhrase->src_words.end - maxDistortion),
		sentLength);
	intersectRange(eSet, t.sourceWordsNotCovered, limit);
    } // if
    UintSet &set(maxDistortion == NO_MAX_DISTORTION ? t.sourceWordsNotCovered : eSet);
    
    vector<vector<PhraseInfo *> > picks;
    pickItemsByRange(picks, phrases, set);
    
    // EJJ Count how many PhraseInfo's we might keep, so that we can
    // pre-allocate the memory for  p, the result vector
    Uint phraseCount = 0;
    for (vector< vector<PhraseInfo *> >::const_iterator it = picks.begin(); 
         it < picks.end(); it++)
    {
        phraseCount += it->size();
    }
    p.reserve(phraseCount);

    // Put all the PhraseInfo's into a single vector, results
    for (vector< vector<PhraseInfo *> >::const_iterator it = picks.begin();
         it < picks.end(); it++)
    {
	for (vector<PhraseInfo *>::const_iterator jt = it->begin(); jt < it->end(); jt++)
	{
	    if (maxDistortion == NO_MAX_DISTORTION || (*jt)->src_words.start <=
		    t.lastPhrase->src_words.end + maxDistortion)
	    {
		p.push_back(*jt);
	    } // if
	} // for
    } // for
} // findPhrases

