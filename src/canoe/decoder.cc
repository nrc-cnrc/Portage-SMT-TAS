/**
 * @author Aaron Tikuisis
 * @file decoder.cc  This file contains the implementation of the core decoder
 * algorithm, as well as a couple wrapper functions.
 *
 * A note about the DecoderState graph created: all paths (translations) begin
 * with a common "empty" state (created by makeEmptyState()), because this
 * makes the decoder code less messy.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "decoder.h"
#include "hypothesisstack.h"
#include "basicmodel.h"
#include "phrasefinder.h"
#include "forced_phrase_finder.h"
#include "cube_pruning_decoder.h"
#include <vector>
#include <iostream>

using namespace std;
using namespace Portage;

namespace Portage
{
   HypothesisStack *runDecoder(BasicModel &model, const CanoeConfig& c,
                               bool usingLev, bool usingSR)
   {
      if (c.bCubePruning)
         return runCubePruningDecoder(model, c, usingLev, usingSR);
      else
         return runStackDecoder(model, c, usingLev, usingSR);
   }

   HypothesisStack *runStackDecoder(BasicModel &model, const CanoeConfig& c,
                                    bool usingLev, bool usingSR)
   {
      const Uint sourceLength = model.getSourceLength();

      // Create the phrase finder
      PhraseFinder* finder(NULL);
      if ( c.forcedDecoding || c.forcedDecodingNZ ) {
         assert(model.getTargetSent());
         finder = new ForcedTargetPhraseFinder(model, *model.getTargetSent());
      } else {
         finder = new RangePhraseFinder(model.getPhraseInfo(), model);
      }

      // If we only need to top hypothesis, there is no point in keeping
      // recombined states, so we discard them on the fly.
      const bool discardRecomb = (!c.masse && !c.latticeOut && !c.nbestOut);

      // The size of the last stack can be reduced in some cases
      Uint last_stack_size = c.maxRegularStackSize;
      if ( discardRecomb )
         last_stack_size = 1;
      else if ( !c.masse && !c.latticeOut && c.nbestSize < c.maxRegularStackSize )
         last_stack_size = c.nbestSize;

      // Create the hypothesis stacks
      HypothesisStack *hStacks[sourceLength + 1];
      for (Uint i = 0; i < sourceLength + 1; i++)
      {
         hStacks[i] = new HistogramThresholdHypStack(model,
               (i == sourceLength ? last_stack_size : c.maxRegularStackSize),
               log(c.pruneThreshold), c.covLimit, log(c.covThreshold),
               c.diversity, c.diversityStackIncrement, discardRecomb);
      } // for

      // Run the decoder algorithm
      runStackDecoder(model, hStacks, sourceLength, *finder, usingLev, usingSR, c.verbosity);

      // Keep the final hypothesis stack
      HypothesisStack *result = hStacks[sourceLength];

      /* - we do this while popping the stacks.
      // Delete all the other hypothesis stacks
      for (Uint i = 0; i < sourceLength; i++)
      {
         assert(hStacks[i]->isEmpty());
         delete hStacks[i];
      } // for
      */

      delete finder;
      return result;
   } // runStackDecoder

   void runStackDecoder(PhraseDecoderModel &model, HypothesisStack **hStacks,
         Uint sourceLength, PhraseFinder &finder,
         bool usingLev, bool usingSR, Uint verbosity)
   {
      // Put the empty hypothesis on the first stack
      //assert(hStacks[0]->isEmpty());
      hStacks[0]->push(makeEmptyState(sourceLength, usingLev, usingSR));
      Uint numStates = 1;
      Uint numPrunedAtPush = 0;
      Uint numPrunedAtPop = 0;
      Uint numRecombined = 0;
      Uint numRecombKept = 0;
      Uint numUnrecombined = 0;
      Uint numRecombPrunedAtPop =0;
      Uint numCovPruned = 0;
      Uint numRecombCovPruned = 0;

      // Iterate through all stacks in ascending order, except the last one
      if (verbosity >= 2) cerr << "HYP: 1" << endl;
      for (Uint i = 0; i < sourceLength; i++)
      {
         if (verbosity >= 2) cerr << "Popping hypothesis stack " << i << endl;
         double topFutureScore = 0;
         double lowestFutureScore = 0;
         Uint states_popped = 0;

         // Iterate through all states on the current stack
         while (!hStacks[i]->isEmpty())
         {
            DecoderState *state = hStacks[i]->pop();
            assert(state->trans->numSourceWordsCovered == i);

            states_popped++;
            if ( states_popped == 1 ) topFutureScore = state->futureScore;
            lowestFutureScore = state->futureScore;

            // Find all phrases that can be added to the state
            vector<PhraseInfo *> addPhrases;
            finder.findPhrases(addPhrases, *(state->trans));
            for (vector<PhraseInfo *>::const_iterator it = addPhrases.begin();
                 it != addPhrases.end(); it++)
            {
               // Combine this phrase with the current state to create a new state.
               DecoderState *newState = extendDecoderState(state, *it, numStates);
               assert(newState->trans->numSourceWordsCovered <= sourceLength);

               if (verbosity >= 3)
               {
                  cerr << "creating hypothesis ";
                  newState->display(cerr, &model, sourceLength);
               } // if

               // Score the new state
               newState->score = state->score + model.scoreTranslation(*newState->trans, verbosity);
               const double dFutureScore = model.computeFutureScore(*newState->trans);
               newState->futureScore = newState->score + dFutureScore;
               if (verbosity >= 3)
                  cerr << "\tscore " << newState->score << " + future score " <<
                     dFutureScore << " = " << newState->futureScore << endl;

               // Add the new state to the appropriate hypothesis stack
               hStacks[newState->trans->numSourceWordsCovered]->push(newState);
            } // while
         } // while
         numPrunedAtPush += hStacks[i]->getNumPrunedAtPush();
         numPrunedAtPop += hStacks[i]->getNumPrunedAtPop();
         numRecombPrunedAtPop += hStacks[i]->getNumRecombPrunedAtPop();
         numRecombined += hStacks[i]->getNumRecombined();
         numRecombKept += hStacks[i]->getNumRecombKept();
         numUnrecombined += hStacks[i]->getNumUnrecombined();
         numCovPruned += hStacks[i]->getNumCovPruned();
         numRecombCovPruned += hStacks[i]->getNumRecombCovPruned();
         delete hStacks[i];
         hStacks[i] = NULL;

         if (verbosity >= 2) cerr
            << "Popped " << states_popped << " states "
            << "SCORES: highest: " << topFutureScore
            << " lowest: " << lowestFutureScore
            << endl
            << "HYP: "
            << numStates << " added, "
            << numPrunedAtPush << " pruned at push, "
            << numPrunedAtPop << "("
            << numRecombPrunedAtPop << ") pruned at pop, "
            << numCovPruned << "("
            << numRecombCovPruned << ") cov pruned, "
            << numRecombined << " merged, "
            << numRecombKept << " kept, "
            << numUnrecombined << " pruned post-merge."
            << endl;

      }
      numRecombined += hStacks[sourceLength]->getNumRecombined();
      numPrunedAtPush += hStacks[sourceLength]->getNumPrunedAtPush();
      Uint numInFinalStack = hStacks[sourceLength]->size();

      if (verbosity >= 2) {
         streamsize saved_precision = cerr.precision();
         cerr.precision(2);
         cerr << "FINAL HYP: "
              << numStates << " added, "
              << numPrunedAtPush << " (" << (100.0*numPrunedAtPush/numStates)
              << "%) pruned at push, "
              << numPrunedAtPop << "(" << numRecombPrunedAtPop << "/"
              << (100.0*numRecombPrunedAtPop/numStates) << "%) pruned at pop, "
              << numCovPruned << "(" << numRecombCovPruned << "/" 
              << (100.0*numRecombCovPruned/numStates) << "%) cov pruned, "
              << numRecombined << " merged, "
              << numRecombKept << "("
              << (100.0*numRecombKept/numStates) << "%) kept, "
              << numUnrecombined << "("
              << (100.0*numUnrecombined/numStates) << "%) pruned post-merge,"
              << endl
              << "     " << numInFinalStack << " left in final stack."
              << endl;
         cerr.precision(saved_precision);
      }
   } // runStackDecoder

} // ends namespace Portage
