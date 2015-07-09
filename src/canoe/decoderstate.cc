/**
 * @author Aaron Tikuisis
 * @file decoderstate.cc  This file contains the implementation of the
 * DecoderState class and the functions makeEmptyState() and
 * extendDecoderState().
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
#include "basicmodel.h"
#include "decoder.h"
#include "phrasetable.h"
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
   delete trans;
   trans = (PartialTranslation*)0xde1e1ed;

   // Iterate through the recombined translations and delete each
   for (vector<DecoderState *>::const_iterator it = recomb.begin();
         it != recomb.end(); it++)
   {
      assert((*it) != NULL);
      assert((*it)->refCount == 0);
      delete *it;
   }

   if (back != NULL)
   {
      // If there is a back-reference, decrement its reference count and
      // delete it if applicable
      assert(back->refCount > 0);
      back->refCount--;
      if (back->refCount == 0)
      {
         delete back;
         back = (DecoderState*)0xde1e1ed;
      }
   }

   // memory trampling detection
   id = 1234000000 + id;
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

void DecoderState::display(ostream& out, PhraseDecoderModel *model, Uint sourceLength) const
{
   if (sourceLength == 0)
      sourceLength = trans->numSourceWordsCovered + countWords(trans->sourceWordsNotCovered);

   out << id;
   if (back) out << " from " << back->id << endl;
   if (back) out << "\tback score            " << back->score << endl;
   out << "\tcoverage              ";
   if (back)
      out << displayUintSet(back->trans->sourceWordsNotCovered, false, sourceLength) << " + ";
   out << trans->lastPhrase->src_words << " = ";
   out << displayUintSet(trans->sourceWordsNotCovered, false, sourceLength) << endl;
   out << "\tnum covered words     " << trans->numSourceWordsCovered << endl;
   if (trans->isLmContextSizeSet())
      out << "\tlm context size       " << trans->getLmContextSize() << endl;
   if (trans->getBiLMContextSize(1) != ArrayUint4::MAX) {
      out << "\tBiLM context size(s) ";
      for (Uint i = 1; i <= ArrayUint4::MAXI; ++i) {
         Uint size = trans->getBiLMContextSize(i);
         if (size == ArrayUint4::MAX) break;
         out << " " << size;
      }
      out << endl;
   }

   if (model) {
      out << "\ttarget phrase         "
          << model->getStringPhrase(trans->lastPhrase->phrase);
      out << endl;
   }

   // display the contents of the annotations stored with the phrase pair.
   trans->lastPhrase->annotations.display(out);
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
   DecoderState *makeEmptyState(Uint sourceLength, bool usingLev, bool usingSR)
   {
      // Create the objects
      DecoderState *state = new DecoderState;
      PartialTranslation *trans = new PartialTranslation(sourceLength, usingLev, usingSR);
      state->trans = trans;

      // Set up the empty state
      state->id = 0;
      state->back = NULL;
      state->score = 0;
      state->futureScore = 0;
      state->refCount = 0;

      return state;
   } // makeEmptyState

   DecoderState *extendDecoderState(DecoderState *state0, const PhraseInfo *phrase,
         Uint &numStates, const UintSet* preCalcSourceWordsCovered)
   {
      assert(state0 != NULL);
      assert(phrase != NULL);

      // Get the translation for state0
      PartialTranslation *trans0 = state0->trans;
      assert(trans0 != NULL);

      // Create the new partial translation, extending trans0
      PartialTranslation *trans =
         new PartialTranslation(trans0, phrase, preCalcSourceWordsCovered);

      // Create the new decoder state
      DecoderState *state = new DecoderState;
      state->trans = trans;

      // Set up references to the previous state
      state->back = state0;

      // Increment state0's reference count and initialize new state to have
      // 0 reference count
      ++state0->refCount;
      state->refCount = 0;

      // Assign a unique id
      state->id = numStates;
      numStates++;

      // to detect uninitialized memory
      state->score = state->futureScore = 1234;

      return state;
   } // extendDecoderState

} // Portage nameSpace
