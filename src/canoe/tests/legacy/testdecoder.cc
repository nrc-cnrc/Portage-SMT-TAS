/**
 * @author Aaron Tikuisis
 * @file testdecoder.cc  This file contains code to test the runDecoder()
 * function.  Currently, the testing is not fully automated; rather, it
 * produces output which must be checked by a user.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "canoe_general.h"
#include "decoder.h"
#include "phrasedecoder_model.h"
#include "hypothesisstack.h"
#include "phrasefinder.h"
#include <vector>
#include "string_hash.h"
#include <iostream>

using namespace std;
using namespace Portage;

namespace Portage {
namespace TestDecoder {

/**
 * A toy PhraseDecoderModel for the test.  The score (and future score)
 * assigned to a translation is precisely the first word in its last phrase
 * (words are represented by unsigned ints).  All other functions of the
 * PhraseDecoderModel are not used by runDecoder().
 */
class TestPDM: public PhraseDecoderModel
{
   virtual string getStringPhrase(const Phrase &uPhrase) { return ""; }
   virtual Uint getUintWord(const string &word) { return 0; }
   virtual Uint getSourceLength() { return 0; }
   virtual vector<PhraseInfo *> **getPhraseInfo() { return NULL; }
   virtual double scoreTranslation(const PartialTranslation &trans, Uint verbosity = 1)
   {
      return trans.lastPhrase->phrase.front();
   }
   virtual double computeFutureScore(const PartialTranslation &trans)
   {
      return 0;
   }
   virtual Uint computeRecombHash(const PartialTranslation &trans) { return 0; }
   virtual bool isRecombinable(const PartialTranslation &trans1, const PartialTranslation &trans2)
   {
      return 0;
   }
   virtual void getFeatureFunctionVals(vector<double> &vals, const PartialTranslation &trans) {}
   virtual void getFeatureWeights(vector<double> &wts) {}
}; // TestPDM

/**
 * A toy HypothesisStack used for the test.  Hypotheses are never recombined
 * and never pruned.
 */
class TestHypStack: public HypothesisStack
{
private:
   vector<DecoderState *> stack;   ///< The initial DecoderStates
   vector<DecoderState *> popped;  ///< The popped DecoderStates
   /**
    * Releases the DecoderStates of a vector.
    * @param v  DecoderStates to delete
    */
   void deleteStates(vector<DecoderState *> &v)
   {
      for ( vector<DecoderState *>::const_iterator it = v.begin();
            it != v.end(); it++)
      {
         assert(*it != NULL);
         assert((*it)->refCount > 0);
         (*it)->refCount--;
         if ((*it)->refCount == 0)
            delete *it;
      }
      v.clear();
   } // deleteStates

public:
   /// Destructor.
   virtual ~TestHypStack()
   {
      deleteStates(stack);
      deleteStates(popped);
   } // ~TestHypStack

   virtual void push(DecoderState *s)
   {
      s->refCount++;
      stack.push_back(s);
   } // push

   virtual DecoderState *pop()
   {
      DecoderState *result = stack.back();
      stack.pop_back();
      popped.push_back(result);
      return result;
   } // pop

   virtual bool isEmpty()
   {
      return stack.empty();
   } // isEmpty

   virtual Uint size() const { return 0; }
   virtual Uint getNumPrunedAtPush() const { return 0; }
   virtual Uint getNumPrunedAtPop() const { return 0; }
   virtual Uint getNumRecombined() const { return 0; }
   virtual Uint getNumUnrecombined() const { return 0; }
   virtual Uint getNumRecombKept() const { return 0; }
   virtual Uint getNumRecombPrunedAtPop() const { return 0; }
   virtual Uint getNumCovPruned() const { return 0; }
   virtual Uint getNumRecombCovPruned() const { return 0; }
}; // TestHypStack

/**
 * A hash function for Range's.
 * Callable entity.
 */
class RangeHash
{
public:
   /**
    * Calculates the hash value for Range r.
    * @param r  range to hash
    * @return Returns the hash value for r
    */
   Uint operator()(const Range &r) const
   {
      return r.start * 100 + r.end;
   } // operator()
}; // RangeHash

/**
 * A toy PhraseFinder used for the test.  The phrase options returned by
 * findPhrases is determined by the first range in the sourceWordsNotCovered,
 * and the mapping is prespecified using an unordered_map.
 */
class TestFinder: public PhraseFinder
{
private:
   /// Definition that associates Range with a PhraseInfo
   unordered_map<Range, vector<PhraseInfo *>, RangeHash> &map;

public:
   /// Constructor.
   /// @param map
   TestFinder(unordered_map<Range, vector<PhraseInfo *>, RangeHash> &map): map(map) {}
   virtual void findPhrases(vector<PhraseInfo *> &p, PartialTranslation &t)
   {
      p = map[t.sourceWordsNotCovered.front()];
   } // findPhrases
}; // TestFinder

/**
 * Prints the contents of a hypothesis stack, popping all the items in order to do so.
 * @param stack	The stack whose contents are popped and printed.
 */
void print(HypothesisStack &stack)
{
   VectorPhrase words;
   while (!stack.isEmpty())
   {
      DecoderState *cur = stack.pop();
      words.clear();
      // Get the phrase (we assume it is no longer than 100 words)
      cur->trans->getLastWords(words, 100);

      // Print the phrase
      cout << "Phrase:" << flush;
      for (vector<Uint>::const_iterator it = words.begin(); it != words.end(); it++)
      {
         cout << " " << *it << flush;
      } // for
      cout << endl;

      // Print the score
      cout << "Score: " << cur->score << endl;
      assert(cur->score == cur->futureScore);
   } // while
} // print
} // ends namespace TestDecoder
} // ends namespace Portage
using namespace Portage::TestDecoder;


/**
 * Program testdecoder's entry point.  Sets up and runs the test.
 * @return Returns 0 if successful.
 */
int main(int argc, char* argv[])
{
   if (argc > 1) {
      cerr << "This tests runDecoder." << endl;
      exit(1);
   }

/*
The graph we're going for (along the top is the number source words covered):

0    1    2    3
0 -> A -> C -> F1
            -> F2
       -> D -> F3
            -> F4
  -> B ------> F5
       ------> F6
  ------> E -> F7
            -> F8

Source words covered at each state:
0: {}
A: {2}
B: {0}
C, E: {1, 2}
D: {0, 2}
F1-F8: {0, 1, 2}

Transition phrases (phrase_index, transition, score = tgt_word, src_words)
(Note: some phrases must be reused)
(0,  0->A,  2, [2, 3) )
(1,  0->B,  4, [0, 1) )
(2,  0->E,  6, [1, 3) )
(3,  A->C,  2, [1, 2) )
(4,  A->D,  4, [0, 1) )
(5,  B->F5, 3, [1, 3) )
(6,  B->F6, 5, [1, 3) )
(7,  C->F1, 2, [0, 1) )
(8,  C->F2, 4, [0, 1) )
(9,  D->F3, 3, [1, 2) )
(10, D->F4, 5, [1, 2) )
(7,  E->F7, 2, [0, 1) )
(8,  E->F8, 4, [0, 1) )

Expected phrases (final state, path, phrase, score = sum of words in phrase):
(F1, 0->A->C->F1, 2 2 2, 6)
(F2, 0->A->C->F2, 2 2 4, 8)
(F3, 0->A->D->F3, 2 4 3, 9)
(F4, 0->A->D->F4, 2 4 5, 11)
(F5, 0->B->F5,    4 3,   7)
(F6, 0->B->F6,    4 5,   7)
(F7, 0->E->F7,    6 2,   8)
(F8, 0->E->F8,    6 4,   10)
*/

   const int NUM_PHRASEOPTIONS = 11;
   const int SRC_LENGTH = 3;
   PhraseInfo phrases[NUM_PHRASEOPTIONS];

   // Hard-code the test input data

   // Create all the phrase options (used as state transitions)
   // 0->A
   phrases[0].src_words.start = 2;
   phrases[0].src_words.end = 3;
   phrases[0].phrase = VectorPhrase(1, 2);

   // 0->B
   phrases[1].src_words.start = 0;
   phrases[1].src_words.end = 1;
   phrases[1].phrase = VectorPhrase(1, 4);

   // 0->C
   phrases[2].src_words.start = 1;
   phrases[2].src_words.end = 3;
   phrases[2].phrase = VectorPhrase(1, 6);

   // A->C
   phrases[3].src_words.start = 1;
   phrases[3].src_words.end = 2;
   phrases[3].phrase = VectorPhrase(1, 2);

   // A->D
   phrases[4].src_words.start = 0;
   phrases[4].src_words.end = 1;
   phrases[4].phrase = VectorPhrase(1, 4);

   // B->F5
   phrases[5].src_words.start = 1;
   phrases[5].src_words.end = 3;
   phrases[5].phrase = VectorPhrase(1, 3);

   // B->F6
   phrases[6].src_words.start = 1;
   phrases[6].src_words.end = 3;
   phrases[6].phrase = VectorPhrase(1, 5);

   // C->F1, E->F7
   phrases[7].src_words.start = 0;
   phrases[7].src_words.end = 1;
   phrases[7].phrase = VectorPhrase(1, 2);

   // C->F2, E->F8
   phrases[8].src_words.start = 0;
   phrases[8].src_words.end = 1;
   phrases[8].phrase = VectorPhrase(1, 4);

   // D->F3
   phrases[9].src_words.start = 1;
   phrases[9].src_words.end = 2;
   phrases[9].phrase = VectorPhrase(1, 3);

   // D->F4
   phrases[10].src_words.start = 1;
   phrases[10].src_words.end = 2;
   phrases[10].phrase = VectorPhrase(1, 5);

   RangeHash rangeHash;
   unordered_map<Range, vector<PhraseInfo *>, RangeHash> finderMap(10, rangeHash);

   Range curRange;
   vector<PhraseInfo *> curVec;

   // Create the map used to specify the available phrase options at each state
   // Transitions from 0
   curRange.start = 0;
   curRange.end = 3;
   curVec.clear();
   curVec.push_back(phrases + 0);
   curVec.push_back(phrases + 1);
   curVec.push_back(phrases + 2);
   finderMap[curRange] = curVec;

   // Transitions from A
   curRange.start = 0;
   curRange.end = 2;
   curVec.clear();
   curVec.push_back(phrases + 3);
   curVec.push_back(phrases + 4);
   finderMap[curRange] = curVec;

   // Transitions from B
   curRange.start = 1;
   curRange.end = 3;
   curVec.clear();
   curVec.push_back(phrases + 5);
   curVec.push_back(phrases + 6);
   finderMap[curRange] = curVec;

   // Transitions from C,E
   curRange.start = 0;
   curRange.end = 1;
   curVec.clear();
   curVec.push_back(phrases + 7);
   curVec.push_back(phrases + 8);
   finderMap[curRange] = curVec;

   // Transitions from D
   curRange.start = 1;
   curRange.end = 2;
   curVec.clear();
   curVec.push_back(phrases + 9);
   curVec.push_back(phrases + 10);
   finderMap[curRange] = curVec;

   TestFinder finder(finderMap);

   // Create the hypothesis stacks
   HypothesisStack *hStacks[SRC_LENGTH + 1];
   for (int i = 0; i < SRC_LENGTH + 1; i++)
   {
      hStacks[i] = new TestHypStack;
   }
   TestPDM model;

   // Run the decoder algorithm
   runDecoder(model, hStacks, SRC_LENGTH, finder, false, false, 1);

   // Delete all stacks but the last one
   for (int i = 0; i < SRC_LENGTH; i++)
   {
      delete hStacks[i];
   }

   // Print the contents of the final stack
   print(*hStacks[SRC_LENGTH]);

   // Delete the final stack
   delete hStacks[SRC_LENGTH];
} // main
