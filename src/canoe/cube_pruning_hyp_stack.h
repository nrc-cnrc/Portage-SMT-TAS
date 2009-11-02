/**
 * @author Eric Joanis
 * @file cube_pruning_hyp_stack.h Hypothesis stack for cube pruning
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technoogies
 * Inst. de technologie de l'information / Institute for Information Technoloy
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __CUBE_PRUNING_HYP_STACK_H__
#define __CUBE_PRUNING_HYP_STACK_H__

#include "hypothesisstack.h"

namespace Portage {

class BasicModel;
class DecoderState;
class PhraseInfo;

/**
 * Helper class for CubePruningHypStack: stores a hyperedge, which represents
 * the edges of a matrix of decoder states, with decoder states all having
 * the same coverage in one dimension, and candidate phrases all have the same
 * range in the other.  Elements of this matrix are represented using
 * HyperedgeItem, but only once they are actually created.
 */
struct Hyperedge {

   /// Model for scoring and recombining states
   BasicModel& model;

   /**
    * The incoming coverage set. This is the coverage set of the partial
    * hypotheses coming into this hyperedge.
    */
   const UintSet sourceWordsNotCovered;

   /**
    * The incoming partial hypothesis list, with the partial heuristic score
    * for each state.  Must be sorted in descending order of partial score.
    * The decoder states are neither owned nor ref counted by the Hyperedge,
    * so the originating stack must still exist for these to be valid.
    */
   const vector<pair<double,DecoderState*> > decoderStates;

   /**
    * The incoming source range.  This is the source range of all phrases
    * that can be added to partial hypotheses in this hyperedge.
    */
   const Range range;

   /**
    * The incoming translation candidates for in_range, with partial heuristic
    * score for each phrase.  Must be sorted in descending order of heuristic
    * score.  The phrase infos are not owned by the Hyperedge, so the
    * originating triangular array must still exist.
    */
   const vector<pair<double,PhraseInfo*> > phrases;

   /**
    * The outgoing coverage set. This is the coverage set that each
    * hypothesis that can come out of this hyperedge will have, calculated
    * once since it will always be the same.
    */
   UintSet out_sourceWordsNotCovered;

   /**
    * This bit matrix indicates which parts of the matrix represented by this
    * Hyperedge have been explored so far.  Specifically, explored[i][j] is
    * true iff HyperedgeItem(this, i, j) has been created.
    */
   vector<boost::dynamic_bitset<> > explored;

   /// Sentence length - copied here for access by HyperedgeItem
   Uint sourceLength;

   /// pointer to state counter - copied here for access by HyperedgeItem
   Uint* nextDecoderStateId;

   /// Verbosity level - copied here for access by HyperedgeItem
   Uint verbosity;

   /// Constructor
   Hyperedge(BasicModel& model,
         const UintSet& sourceWordsNotCovered,
         const vector<pair<double, DecoderState*> >& decoderStates,
         const Range range,
         const vector<pair<double, PhraseInfo*> > & phrases,
         Uint sourceLength,
         Uint* nextDecoderStateId,
         Uint verbosity)
      : model(model)
      , sourceWordsNotCovered(sourceWordsNotCovered)
      , decoderStates(decoderStates)
      , range(range)
      , phrases(phrases)
      , explored(decoderStates.size(), boost::dynamic_bitset<>(phrases.size()))
      , sourceLength(sourceLength)
      , nextDecoderStateId(nextDecoderStateId)
      , verbosity(verbosity)
   {
      subRange(out_sourceWordsNotCovered, sourceWordsNotCovered, range);
   }

   /**
    * Display all elements of the hyperedge with various relevant scores.
    * Very verbose output for data analysis - very expensive to run, since it
    * constructs and evaluates all hyperedge items
    * @param s The index of the stack this hyperedge belongs to, i.e., the
    *          number of words covered in its output states.
    */
   void DisplayAllItems(Uint s);
}; // Hyperedge

/**
 * A fully evaluated element in a Hyperedge matrix.
 */
class HyperedgeItem {
   public:

      /// Pointer to the Hyperedge this item belongs to
      Hyperedge* e;
      /// This item's fully expanded and scored resulting decoder state.
      /// Not owned nor ref counted by this object.
      DecoderState* ds;
      /// This item's position along e's first dimension: decoder states.
      Uint state_index;
      /// This item's position along e's second dimension: candidate phrases.
      Uint phrase_index;
      /// Heuristic score of this item
      double heuristic_score;

      /**
       * Callable object to sort HyperedgeItem's by their decoder state score.
       * Based on the DecoderState WorseScore object, to benefit from the
       * same stable tie breaking mechanism.
       */
      struct ScoreLessThan : private Portage::WorseScore {
         /// Return true iff item1's decoder states scores worse than item2's.
         bool operator()(const HyperedgeItem* item1, const HyperedgeItem* item2)
         { return Portage::WorseScore::operator()(item1->ds, item2->ds); }
      };

      /**
       * Constructor.
       * Calculates the heuristic_score for this item.
       * @param e                    The Hyperedge this item belongs to
       * @param state_index          This item's position along e's first
       *                             dimension: decoder states.
       * @param phrase_index         This item's position along e's second
       *                             dimension: candidate phrases.
       * @param create_decoder_state if true, also allocate the decoder state
       *                             by calling CreateDecoderState().
       */
      HyperedgeItem(Hyperedge* e, Uint state_index, Uint phrase_index,
                    bool create_decoder_state);

      /**
       * Create the decoder state for this item and fully scores it.  After
       * calling this method, it is the caller's responsibility to delete
       * this->ds when it is no longer needed.
       */
      void CreateDecoderState();

      /**
       * Get the up to two successors of this item which have not been created
       * yet.
       * @param items  The successors will be pushed back onto this vector
       * @param create_decoder_states  If true, decoder states will be created,
       *               otherwise the items will only have their heuristic
       *               scores calculated.
       */
      void getAllSuccessors(vector<HyperedgeItem*>& items,
                            bool create_decoder_states);

}; // HyperedgeItem

/**
 * Hypothesis stack designed to support the cube pruning algorithm.
 * Normal usage:
 *  1) call add() on all the incoming hyperdges.
 *  2) call KBest() to find the K best states among these hyperedges.
 *  3) call getAllStates() to get those K best states for further processing.
 * For the first stack, you should call push() on an empty state instead of
 * calling add() in step 1).
 *
 * Note: although this is a type of HypothesisStack in a philosophical sense,
 * the interface is quite different from normal phrase decoding stacks.
 * However, for easiest compatibility with canoe, we still subclass
 * HypothesisStack; the overridden methods pop(), empty() and size() are not
 * called during cube pruning, only after.
 */
class CubePruningHypStack : public HypothesisStack {
      /// Model for scoring and recombining states
      BasicModel& model;
      /// Incoming hyperedges
      vector<Hyperedge*> hyperedges;
      /// Best states, in descending order of score within each coverage set
      map<UintSet, vector<DecoderState*> > best_states;
      /// When using pop(), index of the next state to pop
      Uint pop_position;
      /// If true, recombined states are discarded as they are added
      bool discardRecombined;

   public:
      /**
       * Constructor.
       * @param model   The BasicModel used to determine if two
       *                translations are recombinable and to score new decoder
       *                states.
       * @param discardRecombined If true, recombined states will be
       *                discarded as they are added, keeping only the
       *                higher scoring state.  Set only if you will not
       *                trying to extract lattices or n-best lists.
       */
      CubePruningHypStack(BasicModel& model, bool discardRecombined)
         : model(model)
         , pop_position(0)
         , discardRecombined(discardRecombined)
      {}

      /**
       * Destructor - for any decoder states left in here, reduces their
       * ref count and deletes those where the ref count comes down to 0.
       */
      virtual ~CubePruningHypStack();

      /**
       * Append one state into the best states structure.
       * Recombination is *not* done here, as it is assumed to have already
       * been done before.
       * @param ds Decoder State to push and keep.
       */
      virtual void push(DecoderState* ds);

      /**
       * Add a hyperedge to this stack
       * @param edge    Hyperedge to add
       * @pre May only be called before calling KBest(), never after.
       */
      void add(Hyperedge* edge) {
         hyperedges.push_back(edge);
      }

      /**
       * Perform Huang and Chiang's KBest procedure to fill this stack.
       * All incoming hyperedges must already have been added.
       * @param K                Number of states to keep
       * @param pruningThreshold Prune any hypothesis with score
       *                         pruningThreshold worse than the best one
       *                         (must be negative)  Use -INFINITY to keep all.
       * @param stack_index      Index of this stack, for verbose output only.
       * @param verbosity        Verbosity level
       */
      void KBest(Uint K, double pruningThreshold,
                 Uint stack_index, Uint verbosity);

      /**
       * Get the set of all states kept by KBest().
       * @pre KBest() must have been called first.
       */
      map<UintSet, vector<DecoderState*> > * getAllStates() {
         return &best_states;
      }

      /**
       * Remove the (next) best state from this stack.
       * Implemented for compatibility with canoe only.
       * @return A pointer to the state popped.
       * @pre This must be the final stack, with all hypotheses covering the
       * complete source sentence.
       */
      virtual DecoderState *pop();

      /**
       * Determines whether there are any further states that can be pop()'d.
       * Implemented for compatibility with canoe only.
       * @return true iff there are no more states to pop.
       * @pre This must be the final stack, with all hypotheses covering the
       * complete source sentence.
       */
      virtual bool isEmpty();

      /**
       * Count the number of states in this stack, not counting recombined ones.
       */
      virtual Uint size() const { return numKept; }

      //@{
      /// Interesting statistics, only valid after having called KBest().
      Uint getNumHyperedges() const { return hyperedges.size(); }
      Uint getNumPotentialStates() const { return numPotentialStates; }
      Uint getNumEvaluatedStates() const { return numEvaluatedStates; }
      Uint getNumRecombined() const { return numRecombined; }
      //@}

      /**
       * Display all elements of all hyperedges with various relevant scores.
       * Very verbose output for data analysis - very expensive to run, since
       * it constructs and evaluates all hyperedge items for all hyperedges.
       * @param s This stack's index, i.e., the number of words covered in its
       *          output states.
       */
      void DisplayAllItems(Uint s) {
         cerr << endl;
         for ( Uint i(0); i < hyperedges.size(); ++i )
            hyperedges[i]->DisplayAllItems(s);
      }

   private:
      //@{
      /// Storage for KBest() statistics
      Uint numPotentialStates;
      Uint numEvaluatedStates;
      Uint numRecombined;
      Uint numKept;
      //@}

}; // CubePruningHypStack



} // Portage

#endif // __CUBE_PRUNING_HYP_STACK_H__
