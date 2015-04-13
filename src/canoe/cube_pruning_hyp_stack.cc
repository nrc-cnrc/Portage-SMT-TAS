/**
 * @author Eric Joanis
 * @file cube_pruning_hyp_stack.cc Implementation of cube pruning hyp stack
 *
 * Technologies langagieres interactives / Interactive Language Technoogies
 * Inst. de technologie de l'information / Institute for Information Technoloy
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include <portage_defs.h>
#include <lazy_stl.h>
#include "canoe_general.h"
#include "decoder.h"
#include "basicmodel.h"
#include "cube_pruning_hyp_stack.h"

using namespace Portage;

HyperedgeItem::HyperedgeItem(Hyperedge* e, Uint state_index, Uint phrase_index,
                             bool create_decoder_state)
   : e(e), ds(NULL), state_index(state_index), phrase_index(phrase_index)
{
   assert(e);
   assert(e->decoderStates.size() > state_index);
   assert(e->phrases.size() > phrase_index);

   // Note in the hyperedge the fact that we created this item
   e->explored[state_index][phrase_index] = true;

   // Calculate the heuristic score for this item: the sum of the partial
   // heuristic scores of its two components.
   heuristic_score =
      e->decoderStates[state_index].first +
      e->phrases[phrase_index].first;

   if ( create_decoder_state ) CreateDecoderState();

   if ( !create_decoder_state && e->verbosity >= 3 ) {
      DecoderState* ds0(e->decoderStates[state_index].second);
      cerr << "Creating hyperedge item"
           << " from " << ds0->id
           << " (" << state_index << "," << phrase_index << ")" << endl
           << "\theuristic score " << heuristic_score << endl
           << "\tprev coverage "
              << displayUintSet(ds0->trans->sourceWordsNotCovered,
                                false, e->sourceLength)
              << endl
           << "\tsource range "
              << e->phrases[phrase_index].second->src_words << endl;
      cerr << "\ttarget phrase "
           << e->model.getStringPhrase(e->phrases[phrase_index].second->phrase)
           << endl;
   }
}

void HyperedgeItem::CreateDecoderState()
{
   DecoderState* ds0(e->decoderStates[state_index].second);
   ds = extendDecoderState(ds0,                             // previous DS
                           e->phrases[phrase_index].second, // phrase
                           *e->nextDecoderStateId,          // unique state counter
                           &(e->out_sourceWordsNotCovered));

   if (e->verbosity >= 3) {
      cerr << "Creating hypothesis ";
      ds->display(cerr, &e->model, e->sourceLength);
   }

   // We fully score ds here, since extendDecoderState doesn't do so
   ds->score = ds0->score + e->model.scoreTranslation(*ds->trans, e->verbosity);
   double dFutureScore = e->model.computeFutureScore(*ds->trans);
   ds->futureScore = ds->score + dFutureScore;

   if (e->verbosity >= 3) {
      cerr << "\thyperedge coordinates "
           << state_index << "," << phrase_index << endl
           << "\theuristic score       " << heuristic_score << endl
           << "\tbase score            " << ds->score << endl
           << "\tfuture score          " << ds->futureScore << endl;

      if (heuristic_score != ds->futureScore)
         cerr << "\tfuture != heuristic" << endl;
   }
}

void HyperedgeItem::getAllSuccessors(vector<HyperedgeItem*>& items,
                                     bool create_decoder_states)
{
   // state successor
   if ( (state_index + 1 < e->decoderStates.size()) &&
         ! e->explored[state_index+1][phrase_index] )
      items.push_back(new HyperedgeItem(e, state_index+1, phrase_index,
                                        create_decoder_states));
   // phrase successor
   if ( (phrase_index + 1 < e->phrases.size()) &&
         ! e->explored[state_index][phrase_index+1] )
      items.push_back(new HyperedgeItem(e, state_index, phrase_index+1,
                                        create_decoder_states));

   /* A quick experiment suggests this is a bad idea - reduced BLEU on GALE 07
    * Chinese text, running cow.sh with a stack of 3000.
   // Add predecessors too, if they haven't been added before
   // state predecessor
   if ( (state_index > 0) &&
         ! e->explored[state_index-1][phrase_index] )
      items.push_back(new HyperedgeItem(e, state_index-1, phrase_index,
                                        create_decoder_states));
   // phrase predecessor
   if ( (phrase_index > 0) &&
         ! e->explored[state_index][phrase_index-1] )
      items.push_back(new HyperedgeItem(e, state_index, phrase_index-1,
                                        create_decoder_states));
   */
}

void Hyperedge::DisplayAllItems(Uint s) {
   if ( phrases.empty() || decoderStates.empty() ) return;

   vector<boost::dynamic_bitset<> > save_explored(explored);

   string s_cov = displayUintSet(sourceWordsNotCovered, false, sourceLength);
   string s_range = range.toString();
   for ( Uint i(0); i < decoderStates.size(); ++i ) {
      PartialTranslation pt(decoderStates[i].second->trans, phrases[0].second,
         &out_sourceWordsNotCovered);
      double partial_score = model.rangePartialScore(pt);
      for ( Uint j(0); j < phrases.size(); ++j ) {
         HyperedgeItem hitem(this, i, j, true);

         cerr << "HITEM " << s << " "           // stack id
              << s_cov << " " << s_range << " " // edge id
              << i << " "                       // item id part 1: state
              << decoderStates[i].second->score << " "
              << decoderStates[i].second->futureScore << " "
              << partial_score << " "
              << (partial_score + decoderStates[i].second->score) << " "
              << j << " "                       // item id part 2: phrase
              << phrases[j].second->phrase_trans_prob << " (";
         phrases[j].second->display();
         cerr << ") "
              << phrases[j].second->partial_score << " "
              << "=> "                          // output
              << hitem.ds->score << " "
              << hitem.ds->futureScore
              << endl;

         delete hitem.ds;
      }
   }

   explored = save_explored;
}

CubePruningHypStack::~CubePruningHypStack() {
   for ( Uint i(0); i < hyperedges.size(); ++i ) {
      delete hyperedges[i];
      hyperedges[i] = NULL;
   }
   for ( map<UintSet, vector<DecoderState*> >::iterator
            cov_it(best_states.begin()), cov_end(best_states.end());
         cov_it != cov_end; ++cov_it ) {
      for ( vector<DecoderState*>::iterator
               state_it(cov_it->second.begin()),
               state_end(cov_it->second.end());
            state_it != state_end; ++state_it ) {
         (*state_it)->refCount--;
         if ( (*state_it)->refCount == 0 )
            delete *state_it;
      }
      cov_it->second.clear();
   }
}

void CubePruningHypStack::push(DecoderState* ds) {
   ds->refCount++;
   const UintSet& cov(ds->trans->sourceWordsNotCovered);
   map<UintSet, vector<DecoderState*> >::iterator it =
      best_states.find(cov);
   if ( it != best_states.end() ) {
      (*it).second.push_back(ds);
   } else {
      best_states.insert(make_pair(cov, vector<DecoderState*>(1, ds)));
   }
}

void CubePruningHypStack::KBest(Uint K, double pruningThreshold,
                                Uint stack_index, Uint verbosity)
{
   if ( verbosity >= 3 )
      cerr << "Starting KBest on stack " << stack_index << ":" << endl;
   if ( hyperedges.empty() ) {
      if ( verbosity >= 2 )
         cerr << "No incoming hyperedges, nothing to do in this stack." << endl;
      numPotentialStates = numEvaluatedStates = numRecombined = numKept = 0;
      return;
   }

   Uint hyperedge_count(hyperedges.size()); // number of incoming hyperedges
   Uint partially_scored_trans(0); // sum of their state dimension
   Uint phrases_count(0); // sum of their phrase dimension
   numPotentialStates = 0; // sum of their number of cells
   numEvaluatedStates = 0; // outgoing states fully scored

   // Initialize the candidate heap
   HyperedgeItem::ScoreLessThan heap_cmp;
   vector<HyperedgeItem*> cand_heap;
   cand_heap.reserve(hyperedges.size() + K);
   for ( Uint i(0); i < hyperedges.size(); ++i ) {
      assert(!hyperedges[i]->decoderStates.empty());
      assert(!hyperedges[i]->phrases.empty());
      cand_heap.push_back(new HyperedgeItem(hyperedges[i], 0, 0, true));
      numEvaluatedStates++;

      partially_scored_trans += hyperedges[i]->decoderStates.size();
      phrases_count += hyperedges[i]->phrases.size();
      numPotentialStates += hyperedges[i]->decoderStates.size() *
                            hyperedges[i]->phrases.size();
   }
   make_heap(cand_heap.begin(), cand_heap.end(), heap_cmp);
   RecombHypStack buffer(model, discardRecombined);
   double best_score = cand_heap.front()->ds->futureScore;

   const Uint sourceLength = hyperedges[0]->sourceLength;

   // Get the K best items off the heap
   Uint pop_count = 0;
   while ( ! cand_heap.empty() && pop_count < K ) {
      HyperedgeItem* item = lazy::pop_heap(cand_heap, heap_cmp);
      ++pop_count;

      if ( item->ds->futureScore >= best_score + pruningThreshold ) {
         // DS passes threshold-based stack pruning
         if ( item->ds->futureScore > best_score )
            best_score = item->ds->futureScore;

         // Apply coverage pruning criteria here.
         //NOT IMPLEMENTED YET

         // If we got this far, we're keeping this item
         if ( verbosity >= 3 ) {
            cerr << "Keeping hypothesis ";
            item->ds->display(cerr, &model, sourceLength);
            cerr << "\thyperedge coordinates "
                 << item->state_index << "," << item->phrase_index << endl
                 << "\tbase score            " << item->ds->score << endl
                 << "\tfuture score          " << item->ds->futureScore << endl;
         }

         // This line must follow the verbose 3 block above, since it might actually
         // delete item->ds under some circumstances.
         buffer.push(item->ds);
      } else {
         if (verbosity >= 3)
            cerr << "Beam pruning " << item->ds->id << " future score " << item->ds->futureScore << endl;
         delete item->ds;
         // EXPERIMENTAL: a futureScore of -INFINITY means late pruning
         // happened using filter features. In that case, we give a chance to
         // that state's neighbours.
         const bool explore_neighbours_of_minus_infinity = true;
         if (!explore_neighbours_of_minus_infinity || item->ds->futureScore != -INFINITY) {
            delete item;
            continue;
         }
      }

      // and we need to expand its neighbours and push them into the heap
      Uint old_size = cand_heap.size();
      item->getAllSuccessors(cand_heap, true);
      vector<HyperedgeItem*>::iterator cand_heap_end(cand_heap.begin()+old_size);
      while ( cand_heap_end < cand_heap.end() ) {
         if (verbosity >= 3)
            cerr << "Pushing hyperedge item " << (*cand_heap_end)->ds->id
                 << " -> (" << (*cand_heap_end)->state_index
                 << "," << (*cand_heap_end)->phrase_index << ") h "
                 << (*cand_heap_end)->heuristic_score << endl;
         ++cand_heap_end;
         push_heap(cand_heap.begin(), cand_heap_end, heap_cmp);
         ++numEvaluatedStates;
      }

      delete item;
   }

   // Clean out unused items left on the candidate heap
   for ( Uint i(0); i < cand_heap.size(); ++i ) {
      delete cand_heap[i]->ds;
      delete cand_heap[i];
   }
   cand_heap.clear();

   // Get the K items kept and place them into their coverage sets, in
   // preparation for making the next set of hyperedges.
   vector<DecoderState*> v_buffer;
   buffer.getAllStates(v_buffer);
   for ( vector<DecoderState*>::iterator it(v_buffer.begin()),
         end(v_buffer.end()); it != end; ++it )
      push(*it);

   numRecombined = buffer.getNumRecombined();
   numKept = buffer.size();

   if (verbosity >= 2)
      cerr << "Done KBest for stack " << stack_index << ":"
           << "\n\tInput hyperedges: " << hyperedge_count
           << "\n\tInput partially scored states: " << partially_scored_trans
           << "\n\tInput candidate phrases: " << phrases_count
           << "\n\tPotential states: " << numPotentialStates
           << "\n\tFully evaluated states: " << numEvaluatedStates
           << "\n\tStates kept: " << numKept
           << " + " << numRecombined << " recombined = " 
           << (numKept + numRecombined)
           << endl;

} // KBest

DecoderState* CubePruningHypStack::pop() {
   assert(!isEmpty());
   if ( ! popStarted ) {
      sort(best_states.begin()->second.begin(),
           best_states.begin()->second.end(),
           Portage::WorseScore());
      reverse(best_states.begin()->second.begin(),
              best_states.begin()->second.end());
      pop_position = 0;
      popStarted = true;
   }
   return best_states.begin()->second[pop_position++];
}

bool CubePruningHypStack::isEmpty() {
   // best_states must be empty or have exactly one item ...
   assert(best_states.size() <= 1);
   // ... covering exactly the full range
   // (i.e., src_words_not_covered is empty)
   assert(best_states.empty() || best_states.begin()->first.empty());
   if ( best_states.empty() )
      return true;
   else
      return pop_position >= best_states.begin()->second.size();
}


