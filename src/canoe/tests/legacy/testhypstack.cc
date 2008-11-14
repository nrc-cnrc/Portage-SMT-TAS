/**
 * @author Aaron Tikuisis
 * @file testhypstack.cc  This file contains code to test implementations of
 * HypothesisStack.  Currently, the testing is not fully automated; rather, it
 * produces output which must be checked by a user.
 *
 * $Id$
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "canoe_general.h"
#include "decoder.h"
#include "hypothesisstack.h"
#include "phrasedecoder_model.h"
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

using namespace std;
using namespace Portage;

namespace Portage {
namespace TestHypothesisStack {

/**
 * A toy PhraseDecoderModel for the test.  Phrases are considered recombinable
 * if they cover the same number of source words.  Functions other than
 * computeRecombHash and isRecombinable should not be used by HypothesisStack
 * objects.
 */
class TestPDM: public PhraseDecoderModel
{
   virtual void getStringPhrase(string &s, const Phrase &uPhrase) {}
   virtual Uint getUintWord(const string &word) { return 0; }
   virtual Uint getSourceLength() { return 0; }
   virtual vector<PhraseInfo *> **getPhraseInfo() { return NULL; }
   virtual double scoreTranslation(const PartialTranslation &trans, Uint verbosity = 1) { return 0; }
   virtual double computeFutureScore(const PartialTranslation &trans) { return 0; }
   virtual Uint computeRecombHash(const PartialTranslation &trans)
   {
      return trans.numSourceWordsCovered;
   } // computeRecombHash
   virtual bool isRecombinable(const PartialTranslation &trans1, const PartialTranslation &trans2)
   {
      return trans1.numSourceWordsCovered == trans2.numSourceWordsCovered;
   } // isRecombinable
   virtual void getFeatureFunctionVals(vector<double> &vals, const PartialTranslation &trans) {}
}; // TestPDM

/**
 * Prints the contents of a hypothesis stack, popping all the items in order to do so.
 * @param stack         The stack whose contents are popped and printed.
 */
void print(HypothesisStack *stack)
{
   while (!stack->isEmpty())
   {
      DecoderState *cur = stack->pop();
      cout << "Score: " << cur->futureScore << endl;
      cout << "Recombs: " << flush;
      for (vector<DecoderState *>::const_iterator it = cur->recomb.begin();
           it != cur->recomb.end(); it++)
      {
         cout << (*it)->score << flush;
      } // for
      cout << endl;
   } // while
   cout << endl;
} // print
} // ends namespace TestHypothesisStack
} // ends namespace Portage
using namespace Portage::TestHypothesisStack;


/**
 * The main function.  Sets up and runs the test.
 */
int main()
{
   TestPDM pdm;
   HypothesisStack *stack;

   const int NUMSTATES = 6;
   Uint numWordsCovered[NUMSTATES];
   double scores[NUMSTATES];

/*
Eqivalence classes (using the "is recombinable" equivalence relation).  Since
two states are considered recombinable iff they have the same number of source
words covered, each equivalence class is specified by the number of source
words covered.  In brackets is the score given to the state.

0 {a(3)}
1 {b(2), d(7)}
2 {c(6), f(4)}
3 {e(5)}
*/

   // a
   numWordsCovered[0] = 0;
   scores[0] = 3;

   // b
   numWordsCovered[1] = 1;
   scores[1] = 2;

   // c
   numWordsCovered[2] = 2;
   scores[2] = 6;

   // d
   numWordsCovered[3] = 1;
   scores[3] = 7;

   // e
   numWordsCovered[4] = 3;
   scores[4] = 5;

   // f
   numWordsCovered[5] = 2;
   scores[5] = 4;

   // In order to produce output after every state added to the stack, we need
   // to create the stack from scratch after each output.
   for (int i = 0; i < NUMSTATES; i++)
   {
      // Create the empty state (to refer back to)
      DecoderState *empty = new DecoderState;
      empty->trans = new PartialTranslation;
      empty->trans->back = NULL;
      empty->back = NULL;
      empty->refCount = 0;

      stack = new HistogramThresholdHypStack(pdm, NO_SIZE_LIMIT,
                                             -3.5, 0, log(0.0));

      // Add the first i states
      for (int j = 0; j <= i; j++)
      {
         DecoderState *cur = new DecoderState;
         cur->trans = new PartialTranslation;
         cur->trans->numSourceWordsCovered = numWordsCovered[j];
         cur->futureScore = cur->score = scores[j];
         cur->refCount = 0;
         cur->back = empty;
         empty->refCount++;
         stack->push(cur);
      }

      // Print the resulting stack
      print(stack);

      delete stack;
   }
} // main
