/**
 * @author Aaron Tikuisis
 * @file decoderstate.cc  This file contains the implementation of the
 * DecoderState class and the functions makeEmptyState() and
 * extendDecoderState().
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
#include "decoder.h"
#include <vector>
#include <iostream>

using namespace std;
using namespace Portage;

// Only one * to prevent double documentation in doxygen
/*
 * Deletes this DecoderState.  The reference count must be 0.  The associated
 * PartialTranslation is also deleted, as well as any recombined ones.  It is
 * thus assumed that these objects are not shared with another DecoderState.
 * The reference count is decremented on the DecoderState referenced by back,
 * and if it's count reaches 0 then it is deleted also.
 */
DecoderState::~DecoderState()
{
    assert(refCount == 0);

    // Delete the translation object
    if ( trans != NULL )
    {
        delete trans;
    }

    // Iterate through the recombined translations and delete each
    for (vector<DecoderState *>::const_iterator it = recomb.begin();
         it != recomb.end(); it++)
    {
        assert((*it) != NULL);
        assert((*it)->refCount == 0);
        delete *it;
    } // for

    if (back != NULL)
    {
        // If there is a back-reference, decrement its reference count and
        // delete it if applicable
        assert(back->refCount > 0);
        back->refCount--;
        if (back->refCount == 0)
        {
            delete back;
        } // if
    } // if
} // ~DecoderState

void DecoderState::swap(DecoderState& other)
{
    std::swap<Uint>(refCount, other.refCount);
    std::swap<Uint>(id, other.id);
    std::swap<double>(score, other.score);
    std::swap<double>(futureScore, other.futureScore);
    std::swap<PartialTranslation*>(trans, other.trans);
    std::swap<DecoderState*>(back, other.back);
}

Uint DecoderState::pruneRecombinedStates(double threshold)
{
    vector<DecoderState *> keepRecomb;
    keepRecomb.reserve(recomb.size());
    Uint count = 0;
    for (vector<DecoderState *>::iterator it = recomb.begin();
         it != recomb.end(); it++)
    {
        assert((*it) != NULL);
        if ((*it)->futureScore > threshold) {
            keepRecomb.push_back(*it);
        } else {
            delete *it;
            count++;
        }
    }
    recomb = keepRecomb;
    return count;
}


namespace Portage
{
    // Only one * to prevent double documentation in doxygen
    /*
     * Creates a DecoderState with an empty partial translation, used as the
     * initial state in the decoder algorithm (and hence, the initial state in
     * all translations).
     * @param sourceLength      The length of the source sentence.
     * @return                  The empty state created.
     */
    DecoderState *makeEmptyState(Uint sourceLength)
    {
        // Create the objects
        DecoderState *state = new DecoderState;
        PartialTranslation *trans = new PartialTranslation;
        trans->lastPhrase = new PhraseInfo;
        state->trans = trans;

        // Set up the empty phrase translation
        trans->lastPhrase->src_words.start = 0;
        trans->lastPhrase->src_words.end = 0;
        trans->lastPhrase->phrase_trans_prob = 0;

        // Set up the empty translation
        trans->back = NULL;
        trans->numSourceWordsCovered = 0;

        // Set the range of words not covered to be the full range of words
        Range fullRange(0, sourceLength);
        trans->sourceWordsNotCovered.push_back(fullRange);

        // Set up the empty state
        state->id = 0;
        state->back = NULL;
        state->score = 0;
        state->futureScore = 0;
        state->refCount = 0;

        return state;
    } // makeEmptyState

    // Only one * to prevent double documentation in doxygen
    /*
     * Creates a DecoderState given by adding the given phrase to the end of
     * the translation in state0.  Also increments the reference count in
     * state0.  Note: the computation here makes use of the condition that the
     * ranges in the sourceWordsNotCovered vector are in ascending order, and
     * naturally it ensures that they are in sorted order in the result.
     * @param state0    A pointer to the initial state being extended (cannot
     *                  be NULL and state0->trans cannot be NULL either).
     * @param phrase    A pointer to the phrase being added to state0 (cannot
     *                  be NULL).
     * @return          The extended state.
     */
    DecoderState *extendDecoderState(DecoderState *state0, PhraseInfo *phrase,
                                     Uint &numStates)
    {
        assert(state0 != NULL);
        assert(phrase != NULL);

        // Get the translation for state0
        PartialTranslation *trans0 = state0->trans;
        assert(trans0 != NULL);

        // Create the new objects
        DecoderState *state = new DecoderState;
        PartialTranslation *trans = new PartialTranslation;
        state->trans = trans;

        // Set up references to the previous state
        state->back = state0;
        trans->back = trans0;
        trans->lastPhrase = phrase;

        // Increment state0's reference count and initialize new state to have
        // 0 reference count
        state0->refCount++;
        state->refCount = 0;

        // Assign a unique id
        state->id = numStates;
        numStates++;

        // Compute foreign words covered
        Range &newWords = phrase->src_words;
        assert(trans0->numSourceWordsCovered + newWords.end - newWords.start > 0);
        trans->numSourceWordsCovered = trans0->numSourceWordsCovered +
                                       newWords.end - newWords.start;

        subRange(trans->sourceWordsNotCovered, trans0->sourceWordsNotCovered, newWords);

        return state;
    } // extendDecoderState

} // nameSpace
