/**
 * @author Aaron Tikuisis
 * @file hypothesisstack.cc  This file contains the implementation of the
 * classes HistogramHypStack and ThresholdHypStack, as well as the
 * helper-classes HypHash and HypEquiv.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "hypothesisstack.h"
#include "decoder.h"
#include "phrasedecoder_model.h"
#include "lazy_stl.h"
#include <iostream>

using namespace std;
using namespace Portage;

HypHash::HypHash(PhraseDecoderModel &model): m(model) {}

Uint HypHash::operator()(const DecoderState *s) const
{
   return m.computeRecombHash(*(s->trans));
} // operator()

HypEquiv::HypEquiv(PhraseDecoderModel &model): m(model) {}

bool HypEquiv::operator()(const DecoderState *s1, const DecoderState *s2) const
{
   return m.isRecombinable(*(s1->trans), *(s2->trans));
} // operator()

bool WorseScore::operator()(const DecoderState *s1, const DecoderState *s2) const
{
   // First comparison criterion: future score
   if ( s1->futureScore < s2->futureScore ) return true;
   else if ( s1->futureScore > s2->futureScore ) return false;
   else {
      // Second comparison criterion, in case of tie: score so far
      if ( s1->score < s2->score ) return true;
      else if ( s1->score > s2->score ) return false;
      else {
         // Third comparison criterion, if still a tie: sequential unique id
         // earlier hypotheses are artitrarily considered "better", so ">"
         // on id means "worse than".
         // With cube pruning, this is not a bad choice because decoder states
         // are expanded is reverse order of heuristics.
         // With regular decoding the ID is not as good a choice, but we always
         // use cube pruning so it's not worth trying to improve on this
         // choice.
         // In all cases, WorseScore must define a complete ordering, and using
         // the id guarantees that.
         return s1->id > s2->id;
      }
   }

   return s1->futureScore < s2->futureScore;
} // operator()

RecombHypStack::RecombHypStack(
      PhraseDecoderModel &model,
      bool discardRecombined,
      bool verbose
)
   : hh(model)
   , he(model)
   , verbose(verbose)
   , recombHash(1, hh, he)
   , discardRecombined(discardRecombined)
   , numRecombined(0)
{}

RecombHypStack::~RecombHypStack()
{
   vector<DecoderState *> statesToDelete;
   getAllStates(statesToDelete);
   recombHash.clear();
   for (vector<DecoderState *>::iterator it = statesToDelete.begin(); it !=
        statesToDelete.end(); it++)
   {
      assert((*it)->refCount > 0);
      (*it)->refCount--;
      if ((*it)->refCount == 0)
         delete (*it);
   }
} // ~RecombHypStack

void RecombHypStack::push(DecoderState *s)
{
   // Check preconditions
   assert(!popStarted);
   assert(s != NULL);
   assert(s->trans != NULL);
   assert(s->recomb.empty());
   assert(s->refCount == 0);

   // Increment the reference count on s since it's being added
   s->refCount++;

   // Determine if there is a state to recombine with
   RecombinedSet::const_iterator finder = recombHash.find(s);

   if (finder == recombHash.end())
   {
      // No existing translation to recombine with
      recombHash.insert(s);
   } else
   {
      // Found a translation to recombine with
      numRecombined++;
      DecoderState *rState = *finder;
      if (verbose) cerr << "\trecombining           " << s->id << " with " << rState->id << nf_endl;
      assert(rState->refCount == 1);

      if (rState->futureScore < s->futureScore)
      {
         // The new state has a better score, so swap them by value (that way, we don't
         // remove anything from the hash table).
         // However, the recomb vector with things in it should stay on rState
         swap(*rState, *s);
      }

      s->refCount--;
      assert(s->refCount == 0);
      assert(s->recomb.empty());
      if ( discardRecombined )
         delete s;
      else
         rState->recomb.push_back(s);
   }
} // push()

void RecombHypStack::getAllStates(vector<DecoderState *> &states)
{
   states.reserve(states.size() + recombHash.size());
   states.insert(states.end(), recombHash.begin(), recombHash.end());
} // getAllStates()

HistogramThresholdHypStack::HistogramThresholdHypStack(
   PhraseDecoderModel &model,
   Uint pruneSize, double relativeThreshold,
   Uint covLimit, double covThreshold,
   Uint diversity, Uint diversityStackIncrement,
   bool discardRecombined
)
   : RecombHypStack(model, discardRecombined)
   , pruneSize(pruneSize)
   , threshold(relativeThreshold)
   , bestScore(-INFINITY)
   , covLimit(covLimit)
   , covThreshold(covThreshold)
   , diversity(diversity)
   , diversityStackIncrement(diversityStackIncrement)
   , numKept(0)
   , numPruned(0)
   , numUnrecombined(0)
   , numRecombKept(0)
   , numCovPruned(0)
   , numRecombCovPruned(0)
{
   assert(relativeThreshold < 0);
}

void HistogramThresholdHypStack::push(DecoderState *s)
{
   if (s->futureScore > bestScore + threshold) {
      bestScore = max(bestScore, s->futureScore);
      RecombHypStack::push(s);
   } else {
      delete s;
      numPruned++;
   }
}

void HistogramThresholdHypStack::beginPop()
{
   getAllStates(heap);
   make_heap(heap.begin(), heap.end(), heapCompare);
   if (heap.size() > 0) {
      if (bestScore != heap.front()->futureScore)
         cerr << bestScore << " " << heap.front()->futureScore << endl;
      assert(bestScore == heap.front()->futureScore);
   }
   popStarted = true;
}

DecoderState *HistogramThresholdHypStack::pop()
{
   if (!popStarted)
      beginPop();

   assert(!isEmpty());
   DecoderState *result = lazy::pop_heap(heap, heapCompare);
   numKept++;

   // ASSERT : result is to be kept according to both histogram pruning (as
   // tested by isEmpty()) and to coverage pruning (because the loop below
   // removes any states that fail before returning result).

   // Deal with coverage pruning, if enabled
   if ( covLimit != 0 or covThreshold != -INFINITY or diversity) {
      // Count this state
      UintSet& cov(result->trans->sourceWordsNotCovered);
      CoverageMap::iterator it = covMap.find(cov);
      if ( it != covMap.end() ) {
         (*it).second.second++;
      } else {
         // Coverage not seen before => this state is best for its cov
         covMap.insert(make_pair(cov, make_pair(result->futureScore, 1)));
      }

      // Eliminate any subsequent states that don't pass coverage pruning
      while ( !isEmpty() ) {
         UintSet& cov(heap.front()->trans->sourceWordsNotCovered);
         CoverageMap::iterator it = covMap.find(cov);
         if ( it == covMap.end() ) {
            #ifdef DEBUG_COV_PRUNING
               cerr << "cov not found " << UINTSETOUT(cov) << endl;
            #endif
            break;
         }
         if (
            // IF the diversity minimum has been met:
            (*it).second.second >= diversity &&
            // AND other pruning criteria have been met:
            (
               // EITHER we've popped at least pruneSize states:
               (pruneSize != NO_SIZE_LIMIT && numKept >= pruneSize) ||
               // OR stack pruning threshold has been met
               (heap.front()->futureScore <= bestScore + threshold) ||
               // OR covLimit is set and was exceeded:
               (covLimit != 0 && (*it).second.second >= covLimit)   ||
               // OR covThreshold is set and was exceeded:
               (heap.front()->futureScore <= (*it).second.first + covThreshold)
            )
         ) {
            // prune this state
            #ifdef DEBUG_COV_PRUNING
               cerr << "cov PRUNING " << UINTSETOUT(cov) << " "
                    << (*it).second.second << " " << heap.front()->futureScore
                    << " " << (*it).second.first << endl;
            #endif
            DecoderState *state_to_prune = lazy::pop_heap(heap, heapCompare);

            numCovPruned++;
            numRecombCovPruned += 1 + state_to_prune->recomb.size();

            recombHash.erase(state_to_prune);
            assert(state_to_prune->refCount > 0);
            state_to_prune->refCount--;
            if (state_to_prune->refCount == 0)
               delete state_to_prune;
         } else {
            // Keep this state and break the coverage pruning loop
            #ifdef DEBUG_COV_PRUNING
               cerr << "cov KEEP    " << UINTSETOUT(cov) << " "
                    << (*it).second.second << " " << heap.front()->futureScore
                    << " " << (*it).second.first << endl;
            #endif
            break;
         }
      }
   }

   // For consistency of results, apply the pruneThreshold to recombined
   // hypotheses at pop-time too, not just before they get pushed on.
   numUnrecombined += result->pruneRecombinedStates(bestScore + threshold);

   numRecombKept += result->recomb.size() + 1;

   return result;
} // pop

bool HistogramThresholdHypStack::isEmpty()
{
   if (!popStarted)
      beginPop();

   if ( diversity )
      return ( diversityStackIncrement != NO_SIZE_LIMIT &&
               pruneSize != NO_SIZE_LIMIT &&
               numKept >= pruneSize + diversityStackIncrement ) ||
             heap.empty();
   else
      return (pruneSize != NO_SIZE_LIMIT && numKept >= pruneSize) ||
             heap.empty() ||
             heap.front()->futureScore <= bestScore + threshold;
}

Uint HistogramThresholdHypStack::size() const
{
   if ( popStarted ) {
      return heap.size();
   } else {
      return RecombHypStack::size();
   }
}

Uint HistogramThresholdHypStack::getNumRecombPrunedAtPop() const
{
   Uint count = 0;
   for ( vector<DecoderState *>::const_iterator it = heap.begin();
         it != heap.end(); it++ ) {
      count += 1 + (*it)->recomb.size();
   }
   return count;
}
