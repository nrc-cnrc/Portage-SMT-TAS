/**
 * @author Aaron Tikuisis
 * @file decoder.h  This file contains a declaration of the DecoderState class
 * (used by the decoder algorithm) and of the decoder algorithm itself.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef DECODER_H
#define DECODER_H

#include "canoe_general.h"
#include "phrasefinder.h"
#include <vector>
#include <algorithm>

using namespace std;

namespace Portage
{
    class PartialTranslation;
    class PhraseInfo;
    class PhraseDecoderModel;
    class HypothesisStack;
    class PhraseFinder;

    /**
     * A structure representing a state in the phrase graph.  This consists of an
     * associated translation and some decoder-specific stuff.
     * Note: there is some inherent redundancy since (if these are being used correctly), then
     * state->back->trans == state->trans->back.
     * The reference count should be used by the hypothesis stack and any other structure
     * that has a lasting reference to a decoder state object, to determine when to delete
     * objects of this type.  Moreover, since when a DecoderState is deleted, so is it's
     * associated PartialTranslation and all recombined DecoderState's, these should not be
     * shared among multiple DecoderState objects.
     * Note: this is implemented in decoderstate.cc
     */
    class DecoderState
    {
   public:
       /**
        * Deletes this DecoderState.  The reference count must be 0.  The
        * associated PartialTranslation is also deleted, as well as any
        * recombined DecoderState's.  It is thus assumed that these objects are
        * not shared with another DecoderState.  The reference count is
        * decremented on the DecoderState referenced by back, and if its count
        * reaches 0 then it is deleted also.
        */
       ~DecoderState();

       /**
        * Swap the contents of *this and other, except for recomb.
        */
       void swap(DecoderState& other);

       /**
        * Delete recombined states which have a futureScore below threshold.
        * Returns the number of recombined states pruned.
        */
       Uint pruneRecombinedStates(double threshold);

       /**
        * The number of (lasting) references to this.  When zero, this should be
        * deleted.
        */
       Uint refCount;

       /**
        * A unique id given to this state.
        */
       Uint id;

       /**
        * The partial translation associated with this state.
        */
       PartialTranslation *trans;

       /**
        * The previous state.
        */
       DecoderState *back;

       /**
        * All the recombined translations associated with this state.
        */
       vector<DecoderState *> recomb;

       /**
        * The total score (log probability) for this state.
        */
       double score;

       /**
        * The estimated future score (log probability) for this state.
        */
       double futureScore;
    }; // DecoderState

    /**
     * Will swap 2 decoderState except for their recombined stack
     * @param     first decoderState
     * @param     second decoderState
     * @return    nothing
     */
    inline void swap(DecoderState& first, DecoderState& second)
    {
       first.swap(second);
    }

    /**
     * Creates a DecoderState with an empty partial translation, used as the
     * initial state in the decoder algorithm (and hence, the initial state in
     * all translations).
     * @param sourceLength The length of the source sentence.
     * @return    The empty state created.
     */
    DecoderState *makeEmptyState(Uint sourceLength);

    /**
     * Creates a DecoderState given by adding the given phrase to the end of the
     * translation in state0.  Also increments the reference count in state0.
     * @param state0 A pointer to the initial state being extended (cannot be NULL and
     *         state0->trans cannot be NULL either).
     * @param phrase A pointer to the phrase being added to state0 (cannot be NULL).
     * @param numStates  counter to assign unique ids
     * @return    The extended state.
     */
    DecoderState *extendDecoderState(DecoderState *state0, PhraseInfo *phrase, Uint
       &numStates);

    /**
     * Runs the decoder algorithm using the given model, with a pruning model
     * that keeps only the pruneSize best states on each hypothesis stack, and
     * only keeps states whose score is greater than the best score plus
     * threshold.  Returns the last hypothesis stack; that is, an hypothesis
     * stack containing the best complete translations.
     * @param model  The PhraseDecoderModel to be used by the decoder.
     * @param pruneSize  The number of states to keep on each hypothesis stack.
     * @param threshold  The relative threshold that states must be above to be
     *         kept on each hypothesis stack.
     *         (must be in log space, and negative)
     * @param covLimit   The max number of states to keep with the same
     *                   coverage
     * @param covThreshold The relative threshold that states must be above to
     *         be kept when they have the exact same coverage
     *         (must be in log space, and negative, or log(0.0), i.e.,
     *         -INFINITY, for no threshold)
     * @param distLimit  The maximum distortion distance allowed between two
     *         words.
     * @param verbosity  Indicates the level of verbosity
     * @return  A pointer to the final hypothesis stack, which contains the
     *         best complete translations.  While this HypothesisStack is
     *         created by this function, it must be deleted externally.
     */
    HypothesisStack *runDecoder(PhraseDecoderModel &model, Uint pruneSize,
       double threshold, Uint covLimit, double covThreshold,
       int distLimit = NO_MAX_DISTORTION, Uint verbosity = 1);

    /**
     * Runs the decoder algorithm.  Uses the hypothesis stacks given, emptying
     * all but the last one.
     * @param model  The PhraseDecoderModel to be used to score phrases.
     * @param hStacks   An array of length (sourceLength + 1) containing
     *         pointers to hypothesis stacks.  Each stack should initially be
     *         empty, and all but the final stack will be empty at the end.
     * @param sourceLength The length of the source sentence being translated.
     * @param phraseFinder Phrases that can be added to the current source 
     * @param verbosity  Indicates the level of verbosity
     */
    void runDecoder(PhraseDecoderModel &model, HypothesisStack **hStacks,
       Uint sourceLength, PhraseFinder &phraseFinder, Uint verbosity);

} // Portage

#endif // DECODER_H
