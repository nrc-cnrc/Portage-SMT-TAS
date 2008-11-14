/**
 * @author Eric Joanis
 * @file cube_pruning_decoder.cc Implementation of cube pruning decoder.
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technoogies
 * Inst. de technologie de l'information / Institute for Information Technoloy
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include <portage_defs.h>
#include "basicmodel.h"
#include "distortionmodel.h"
#include "decoder.h"
#include "cube_pruning_decoder.h"
#include "cube_pruning_hyp_stack.h"

namespace Portage {

struct PartialScoreGreaterThan : public Portage::WorseScore {
   /// Return true iff item1 is better than item2
   bool operator()(const pair<double, DecoderState*>& item1,
                   const pair<double, DecoderState*>& item2)
   {
      if ( item1.first < item2.first )
         return false;
      else if ( item1.first > item2.first )
         return true;
      else
         return !Portage::WorseScore::operator()(item1.second, item2.second);
   }
};

/**
 * Take the best states from a hypothesis stack, and combine them with all
 * compatible candidate phrases to form hyperedges into subsequent hypothesis
 * stacks.
 * @param model         The model used to score hypotheses
 * @param stacks        All stacks - must have size sourceLength+1
 * @param sourceLength  Number of tokens in source sentence to decode
 * @param s             Index of the stack from which to make hyperedges; also
 *                      corresponds to the number of words covered by all
 *                      hypotheses we're making hyperedges for.
 * @param scored_phrases Triangular array of scored candidate phrases, where
 *                      the score is the partial heuristic score for each
 *                      phrase.
 * @param nextDecoderStateId pointer to unique state id counter
 * @param verbosity     The verbosity level
 */
static void MakeHyperedges(
   BasicModel &model,
   Uint sourceLength,
   CubePruningHypStack **stacks,
   Uint s,
   vector<pair<double,PhraseInfo*> > **scored_phrases,
   Uint* nextDecoderStateId,
   Uint verbosity
) {
   Uint hyperedge_count(0); // number of hyperedges created
   Uint partially_scored_trans(0); // sum of their state dimension
   Uint phrases_count(0); // sum of their phrase dimension
   Uint total_hyperedge_size(0); // sum of their number of cells
   if ( verbosity >= 3 )
      cerr << "Starting MakeHyperedges on stack " << s << ":" << endl;
   // Declare these vectors outside the loops to reduce the number of
   // alloc/free cycles
   vector<pair<double, DecoderState*> > scored_states;
   vector<vector<pair<double,PhraseInfo*> > > picks;
   UintSet out_cov;
   // Iterate over coverages
   for ( map<UintSet, vector<DecoderState*> >::iterator
            cov_it(stacks[s]->getAllStates()->begin()),
            cov_end(stacks[s]->getAllStates()->end());
         cov_it != cov_end; ++cov_it ) {
      const UintSet& cov(cov_it->first);
      vector<DecoderState*>& states((*cov_it).second);

      // Iterate over source ranges compatible with the coverage (partially
      // copied from RangePhraseFinder::findPhrases(), but adjusted for cube
      // pruning)
      //vector<vector<PhraseInfo*> > picks;
      picks.clear();
      pickItemsByRange(picks, scored_phrases, cov);
      for ( vector<vector<pair<double,PhraseInfo*> > >::iterator
               pick_it(picks.begin());
            pick_it != picks.end(); ++pick_it ) {
         if ( pick_it->empty() ) continue;

         Range src_words(pick_it->front().second->src_words);
         subRange(out_cov, cov, src_words);
         
         // Future score for all states + phrase pair in this hyperedge will be
         // the same, since they have the same resulting coverage and last
         // source word covered.  We calculate it with the first state that
         // passes the distortion limit criteria.
         double futureScore(0.0);
         bool futureScoreCalculated(false);

         //vector<pair<double, DecoderState*> > scored_states;
         //scored_states.reserve(states.size());
         scored_states.clear();

         // Iterate over all incoming states, to partially score them, taking
         // into account their accumulated score, their partial score
         // when we add src_words to them, the resulting future score, but not
         // the phrase score or the combination score.
         for ( Uint i(0); i < states.size(); ++i ) {
            // Skip this decoder state if it fails on the distortion limit
            if ( !
                 (
                    DistortionModel::respectsDistLimit(cov, 
                       states[i]->trans->lastPhrase->src_words,
                       src_words, model.c->distLimit, sourceLength,
                       model.c->distLimitExt, &out_cov)
                  ||
                   (model.c->distPhraseSwap &&
                    DistortionModel::isPhraseSwap(cov, 
                       states[i]->trans->lastPhrase->src_words,
                       src_words, sourceLength, scored_phrases))
                 )
               )
               continue;

            assert(states[i]->trans);
            assert(states[i]->trans->lastPhrase);

            // We use the first phrase in the candidate list to call
            // rangePartialScore, but only its src_words is used, not its
            // translation
            PartialTranslation pt(states[i]->trans,
                                  pick_it->front().second, &out_cov);
            if ( !futureScoreCalculated ) {
               futureScore = model.computeFutureScore(pt);
               futureScoreCalculated = true;
            } else {
               // expensive, keep it only for early debugging!
               /*
               if ( abs(futureScore - model.computeFutureScore(pt)) > 0.00001 )
                  cerr << "futureScore = " << futureScore
                       << " model.computeFutureScore(pt) = "
                       << model.computeFutureScore(pt)
                       << endl;
               */
               //assert(futureScore == model.computeFutureScore(pt));
            }
            double partial_score =
               states[i]->score + model.rangePartialScore(pt) + futureScore;

            scored_states.push_back(make_pair(partial_score, states[i]));
         }

         if ( scored_states.empty() ) {
            // Don't create the hyperedge if no states passed the distortion
            // limit
            if ( verbosity >= 3 )
               cerr << "  Discarding "
                    << displayUintSet(cov, false, sourceLength)
                    << " x " << src_words.toString()
                    << ": " << scored_states.size() << "/" << states.size()
                    << " states, " << pick_it->size() << " phrases"
                    << endl;
            continue;
         }

         // Sort the states in descending order of this partial score for the
         // purpose of hyperedge creation.
         sort(scored_states.begin(), scored_states.end(),
              PartialScoreGreaterThan());

         // Push them in reverse order into a hyperedge
         Hyperedge* edge = new Hyperedge(model,
                                         cov,
                                         scored_states,
                                         src_words,
                                         *pick_it,
                                         sourceLength,
                                         nextDecoderStateId,
                                         verbosity);

         // Add this hyperedge onto the right stack.
         Uint next_stack = s + src_words.end - src_words.start;
         assert(next_stack <= sourceLength);
         stacks[next_stack]->add(edge);

         if ( verbosity >= 2 ) {
            hyperedge_count += 1;
            partially_scored_trans += scored_states.size();
            phrases_count += pick_it->size();
            total_hyperedge_size += scored_states.size() * pick_it->size();
         }

         if ( verbosity >= 3 )
            cerr << "  Hyperedge "
                 << displayUintSet(cov, false, sourceLength)
                 << " x " << src_words.toString()
                 << ": " << scored_states.size() << "/" << states.size()
                 << " states, " << pick_it->size() << " phrases"
                 << endl;
      }
   }

   if ( verbosity >= 2 )
      cerr << "Done MakeHyperedges on Stack " << s << ":"
           << "\n\tHyperedges out: " << hyperedge_count
           << "\n\tPartially scored states: " << partially_scored_trans
           << "\n\tCandidate phrases: " << phrases_count
           << "\n\tTotal potential next states: " << total_hyperedge_size
           << endl;
} // MakeHyperedges

HypothesisStack* runCubePruningDecoder(BasicModel &model, const CanoeConfig& c)
{
   // Get all the phrase translation options from the model
   Uint sourceLength = model.getSourceLength();
   vector<PhraseInfo*> **phrases = model.getPhraseInfo();
   vector<pair<double, PhraseInfo*> > **scored_phrases =
      TriangArray::Create<vector<pair<double,PhraseInfo*> > >()(sourceLength);

   // Pre-sort the PhraseInfos in decreasing order of their local future score.
   for ( Uint i(0); i < sourceLength; ++i ) {
      for ( Uint j(0); j < sourceLength - i; ++j ) {
         // We are now dealing with source range [i, i+j+1)
         Range source_range(i, i+j+1);

         // Store the phrase info pointers and local future scores into a
         // vector we can sort.
         //vector<pair<double, PhraseInfo*> > scored_phrases;
         scored_phrases[i][j].reserve(phrases[i][j].size());
         for ( vector<PhraseInfo*>::const_iterator it(phrases[i][j].begin()),
               end(phrases[i][j].end()); it != end; ++it ) {
            assert((*it)->src_words == source_range);
            double score = model.phrasePartialScore(*it);
            scored_phrases[i][j].push_back(make_pair(score, *it));
         }

         // Sort these phrase info pointers in a stable way using the same
         // (arbitrary but stable) tie breaking rules as for picking the L best
         // phrase table entries.
         sort(scored_phrases[i][j].begin(), scored_phrases[i][j].end(),
              PhraseTable::PhraseScorePairGreaterThan(model.getPhraseTable()));
      }
   }

   // Create the cube pruning hypothesis stacks
   CubePruningHypStack *stacks[sourceLength + 1];
   for ( Uint i(0); i < sourceLength + 1; ++i )
      stacks[i] = new CubePruningHypStack(model);

   // Calculate the log(c.pruneThreshold) only once
   double threshold = log(c.pruneThreshold);

   // Empty start state for all translations.
   stacks[0]->push(makeEmptyState(sourceLength));
   Uint nextDecoderStateId(1); // 0 is the empty state just created.

   for ( Uint s(1); s <= sourceLength; ++s ) {
      // Make all the hyperedges coming out of the previous stack.
      MakeHyperedges(model, sourceLength, stacks, s-1,
                     scored_phrases, &nextDecoderStateId, c.verbosity);
      // Massively verbose and costly output with verbosity >= 4
      if ( c.verbosity >= 4 ) stacks[s]->DisplayAllItems(s);
      // Construct the next stack by applying KBest on its incoming hyperedges
      stacks[s]->KBest(c.maxStackSize, threshold, s, c.verbosity);
   }
      
   if ( c.verbosity >= 2 ) {
      Uint hyperedge_count(0), potential_states(0), evaluated_states(0),
           kept_states(0), recomb_states(0);
      for ( Uint s(1); s <= sourceLength; ++s ) {
         hyperedge_count += stacks[s]->getNumHyperedges();
         potential_states += stacks[s]->getNumPotentialStates();
         evaluated_states += stacks[s]->getNumEvaluatedStates();
         kept_states += stacks[s]->size();
         recomb_states += stacks[s]->getNumRecombined();
      }

      cerr << "Final statistics, with source length: " << sourceLength
           << "\n\tFinal Hyperedges: " << hyperedge_count
           << "\n\tFinal Potential states: " << potential_states
           << "\n\tFinal Fully evaluated states: " << evaluated_states
           << "\n\tFinal States kept: " << kept_states
           << " + " << recomb_states << " recombined = "
           << (kept_states + recomb_states)
           << endl;
      cerr << "FSTATS\t" << sourceLength
           << "\t" << hyperedge_count
           << "\t" << potential_states
           << "\t" << evaluated_states
           << "\t" << kept_states
           << "\t" << recomb_states
           << "\t" << (kept_states + recomb_states)
           << endl;
   }

   // Clean out the scored phrases
   TriangArray::Delete<vector<pair<double, PhraseInfo*> > >()(
      scored_phrases, sourceLength);

   // Clean out all the stacks, except the last one, which we return 
   for ( Uint s(0); s < sourceLength; ++s )
      delete stacks[s];

   return stacks[sourceLength];
} // runCubePruningDecoder

} // Portage
