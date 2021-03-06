/**
 * @author Aaron Tikuisis
 * @file hypothesisstack.h  This file contains the definition of the abstract
 * class HypothesisStack, as well as the definition of HistogramHypStack and
 * ThresholdHypStack, for the two different types of pruning.
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#ifndef HYPOTHESISSTACK_H
#define HYPOTHESISSTACK_H

#include "canoe_general.h"
#include <logging.h>
#include <vector>
#include <tr1/unordered_set>
#include <map>

using namespace std;

namespace Portage
{
   using tr1::unordered_set;
   class DecoderState;
   class PhraseDecoderModel;

   /**
    * Abstract class representing a hypothesis stack.  Implicitly, the
    * hypothesis stack should take care of both pruning and recombining
    * states.  Due to both pruning and recombining, it is possible (even
    * likely) that more states will be pushed onto the stack than will be
    * popped from the stack before it is empty.  The HypothesisStack must
    * correctly use the refCount of DecoderState's; that is, it should
    * increment the count when a state is added via push, and it should
    * decrement the reference count (and delete if refCount == 0) of all
    * DecoderState's added in the destructor.  A hypothesis stack should have
    * all states pushed onto it BEFORE any states are popped from it; it is up
    * to the child classes to enforce this, but the boolean variable
    * popStarted may be used.
    */
   class HypothesisStack
   {
      protected:
         /**
          * Whether any states have been pop()ed yet
          */
         bool popStarted;

      public:
         /**
          * Constructor.  Initializes popStarted to false.
          */
         HypothesisStack(): popStarted(false) {}

         /**
          * Destructor.  Should decrement reference count and delete all
          * states whose reference count is 0.
          */
         virtual ~HypothesisStack() {}

         /**
          * Adds the given state to this stack and increments its reference
          * count.  If s is recombined into a superior state (either
          * immediately or later) then it is deleted.
          * @param s A pointer to the new state to add (must not be NULL;
          *          s->trans must not be NULL either; s->recomb must be
          *          empty; s->refCount must be 0).
          */
         virtual void push(DecoderState *s) = 0;

         /**
          * Removes a state from this stack and returns a pointer to it.
          * The reference count on the state is not, however, decremented
          * until this stack is deleted.
          * @return  A pointer to the state popped.
          */
         virtual DecoderState *pop() = 0;

         /**
          * Determines whether there are any further states that can be
          * removed from this stack.
          * @return  true iff there are no more states to pop.
          */
         virtual bool isEmpty() = 0;

         /**
          * Gets the number of states in the stack.
          * @return The number of states in the stack
          */
         virtual Uint size() const = 0;

         //@{
         /**
          * Determines the number of states that were pruned.
          * @return  The number of states that were pruned.
          */
         virtual Uint getNumPruned() const {
            return getNumPrunedAtPush() + getNumPrunedAtPop();
         }
         virtual Uint getNumPrunedAtPush() const { return 0; }
         virtual Uint getNumPrunedAtPop() const { return 0; }
         //@}

         // The following shouldn't belong here, it should only be in the
         // subclass, but since the decoder keeps only a pointer the this
         // base class, we need these virtual functions here.

         //@{
         /**
          * Determines the number of states that were recombined.
          * @return  The number of states that were recombined.
          */
         virtual Uint getNumRecombined() const { return 0; }
         virtual Uint getNumRecombKept() const { return 0; }
         //@}

         //@{
         /**
          * Determines the number of states that were recombined but later
          * pruned
          * @return The number of states the were recombined but later
          * pruned.
          */
         virtual Uint getNumUnrecombined() const { return 0; }
         virtual Uint getNumRecombPrunedAtPop() const { return 0; }
         //@}

         //@{
         /**
          * Coverage pruning statistics.
          * @return Retunrs the number of states pruned during coverage.
          */
         virtual Uint getNumCovPruned() const { return 0; }
         virtual Uint getNumRecombCovPruned() const { return 0; }
         //@}
   }; // HypothesisStack


   //--------------------------------------------------------------------------
   /**
    * The following class encapsulates the hash (recombinable) function used
    * to recombine hypotheses in RecombHypStack
    */
   class HypHash
   {
      private:
         PhraseDecoderModel &m;  ///< Model to be used for hashing
      public:
         /**
          * Creates a new HypHash using the given model.
          * @param model   The model used to get hash values.
          */
         HypHash(PhraseDecoderModel &model);
         /**
          * Returns a hash value for the given state.
          * @param s The state to get a hash value for.
          * @return  The hash value.
          */
         Uint operator()(const DecoderState *s) const;
   }; // HypHash


   //--------------------------------------------------------------------------
   /**
    * The following  class encapsulates the equivalence (recombinable)
    * function used to recombine hypotheses in RecombHypStack
    */
   class HypEquiv
   {
      private:
         /// The model used to determine if two translations are equivalent
         /// (recombinable).
         PhraseDecoderModel &m;
      public:
         /**
          * Creates a new HypEquiv using the given model.
          * @param model   The model used to determine if two translations are
          *                equivalent (recombinable).
          */
         HypEquiv(PhraseDecoderModel &model);
         /**
          * Determines if two states are equivalent (the translations are
          * recombinable)
          * @param s1   The first state.
          * @param s2   The second state.
          * @return  true iff the two states have recombinable translations.
          */
         bool operator()(const DecoderState *s1, const DecoderState *s2) const;
   }; // HypEquiv


   //--------------------------------------------------------------------------
   /**
    * Callable entity to sort decoder states based on their estimated cost.
    */
   class WorseScore
   {
      public:
         /**
          * Determines if the translation produce by s1 will be less
          * expensive than s2 to produce.
          * @param s1  left-hand side operand
          * @param s2  right-hand side operand
          * @return Returns true if s1 seems to be the partial translation
          * the least expensive (cost).
          */
         bool operator()(const DecoderState *s1, const DecoderState *s2) const;
   }; // WorseScore


   //--------------------------------------------------------------------------
   /**
    * Hypothesis stack that recombines their states.
    * Recombination occurs if they following are verified:
    * - the foreign words covered so far.
    * - the last two English words genereated.
    * - the end of the last foreign phrase covered.
    */
   class RecombHypStack: public HypothesisStack
   {
      private:
         /// The hash function.
         HypHash hh;

         /// The equivalence (recombinable) function.
         HypEquiv he;

         /// Whether to log recombination operations to stderr
         bool verbose;

      protected:
         /// Definition of a Recombined set of DecoderStates.
         typedef unordered_set<DecoderState *, HypHash, HypEquiv>  RecombinedSet;
         /**
          * The hash set used to track recombinable states.
          */
         RecombinedSet recombHash;

         /// If true, recombined states are discarded as they are added
         bool discardRecombined;

      private:
         /// Keeps track of how many states were recombined.
         Uint numRecombined;

      public:
         /**
          * Creates a new RecombHypStack.
          * @param model     The PhraseDecoderModel used to determine if two
          *                  translations are recombinable.
          * @param discardRecombined If true, recombined states will be discarded
          *                  as they are added, keeping only the higher scoring
          *                  state.  Set only if you will not trying to extract
          *                  lattices or n-best lists.
          * @param verbose   If true, log recombining operations to STDERR
          */
         RecombHypStack(PhraseDecoderModel &model, bool discardRecombined,
                        bool verbose = false);
         /// Destructor.
         virtual ~RecombHypStack();

         virtual void push(DecoderState *s);
         virtual Uint getNumRecombined() const { return numRecombined; };

         /// error to call this method on a RecombHypStack.
         virtual DecoderState* pop() { assert(false); return NULL; }
         /// Implemented only so class is not abstract.
         virtual bool isEmpty() { return recombHash.empty(); }

         /**
          * Puts all the states into the given vector.
          * @param states  returned vector containing all states
          */
         virtual void getAllStates(vector<DecoderState *> &states);
         /**
          * Gets the number of unique states once they are recombined.
          * @copydoc HypothesisStack::size() const
          */
         virtual Uint size() const { return recombHash.size(); };
   }; // RecombHypStack


   //------------------------------------------------------------------------
   /**
    * Hypothesis stack that keeps track of how many operations were done.
    */
   class HistogramThresholdHypStack: public RecombHypStack
   {
      private:
         /**
          * The maximum number of states to keep.
          */
         Uint pruneSize;

         /**
          * The relative threshold; all states with future score below
          * (threshold + best score) are thrown out.
          * Recombined states are removed at pop time if they don't statisfy
          * this criterion.
          */
         double threshold;

         /**
          * The best future score out of all the states pushed on to this
          * stack.
          */
         double bestScore;

         /**
          * The max number of states to keep with the same coverage,
          * i.e., which cover exactly the same source words
          */
         Uint covLimit;

         /**
          * The relative threshold that states must be above to be kept when
          * they have the exact same coverage; all states with future score
          * below (threshold + best score for the same coverage) are thrown
          * out.
          */
         double covThreshold;

         /**
          * Definition of a map used to implement coverage pruning: for each
          * coverage, shows the best score and and the number of states
          * popped so far having this coverage.
          * map : coverage set -> { best score, pop count }
          */
         typedef std::map<UintSet, pair<double, Uint> > CoverageMap;
         CoverageMap covMap;  ///< Map used to implement coverage pruning

         /**
          * Diversity: the minimum number of states to keep for each coverage
          */
         Uint diversity;

         /**
          * Diversity stack increment: the max number of states per stack
          * added because of the diversity criterion.
          */
         Uint diversityStackIncrement;

         /**
          * Comparison functor for the heap
          */
         WorseScore heapCompare;

         /**
          * The heap used to store the states once popping begins, so that
          * they may be popped in order from best to worst.
          */
         vector<DecoderState *> heap;

         /**
          * The number of hypotheses that have been popped so far.
          */
         Uint numKept;

         /**
          * The number of hypotheses that were pruned before even being
          * added.
          */
         Uint numPruned;

         //@{
         /**
          * The number of recombined hypotheses that we filtered out at pop
          */
         Uint numUnrecombined;
         Uint numRecombKept;
         //@}

         //@{
         /**
          * The number of hypotheses, and recombined hypotheses, that were
          * coverage pruned
          */
         Uint numCovPruned;
         Uint numRecombCovPruned;
         //@}

         /**
          * Prepares the DecoderStates in the form of a heap
          */
         void beginPop();

      public:

         /**
          * Creates a new HistogramThresholdHypStack.
          * @param model     The PhraseDecoderModel used to determine if two
          *                  translations are recombinable.
          * @param pruneSize The maximum number of states to keep; all but
          *                  the pruneSize best states are pruned out.  If
          *                  equal to NO_PRUNE_SIZE, then there is no
          *                  maximum number of states to keep.
          * @param relativeThreshold  The relative threshold (logarithmic).
          *                  That is, all states whose future score is less
          *                  than (best score + relativeThreshold) are
          *                  pruned out.
          * @param covLimit  The max number of states to keep with the same
          *                  coverage
          * @param covThreshold The relative threshold that states must be
          *                  above to be kept when they have the exact same
          *                  coverage (must be in log space, and negative,
          *                  or log(0.0), i.e., -INFINITY, for no threshold)
          * @param diversity The minimum number of states to keep per coverage
          * @param diversityStackIncrement the max number of states per stack
          *                  added because of the diversity criterion.
          * @param discardRecombined If true, recombined states will be discarded
          *                  as they are added, keeping only the higher scoring
          *                  state.  Set only if you will not trying to extract
          *                  lattices or n-best lists.
          */
         HistogramThresholdHypStack(PhraseDecoderModel &model,
               Uint pruneSize, double relativeThreshold,
               Uint covLimit, double covThreshold, Uint diversity,
               Uint diversityStackIncrement, bool discardRecombined);

         // Methods overridden from parent classes
         virtual void push(DecoderState *s);
         virtual DecoderState *pop();
         virtual bool isEmpty();
         virtual Uint size() const;
         virtual Uint getNumPrunedAtPush() const { return numPruned; }
         virtual Uint getNumPrunedAtPop() const { return heap.size(); }
         virtual Uint getNumUnrecombined() const { return numUnrecombined; }
         virtual Uint getNumRecombKept() const { return numRecombKept; }
         virtual Uint getNumRecombPrunedAtPop() const;
         virtual Uint getNumCovPruned() const { return numCovPruned; }
         virtual Uint getNumRecombCovPruned() const { return numRecombCovPruned; }
   }; // class HistogramThresholdHypStack

} // namespace Portage

#endif // HYPOTHESISSTACK_H
