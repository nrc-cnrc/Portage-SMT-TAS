/**
 * @author Aaron Tikuisis
 * @file wordgraph.cc  This file contains the implementation of the
 * writeWordGraph() function, used to create lattice output that may be used by
 * your favorite lattice processing package.
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

#include <wordgraph.h>
#include <decoder.h>
#include <phrasedecoder_model.h>
#include <basicmodel.h>
#include <math.h>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/dynamic_bitset.hpp>
#include <boost/shared_ptr.hpp>


using namespace Portage;
using namespace std;
using namespace __gnu_cxx;


typedef boost::dynamic_bitset<> SEEN;
typedef vector<DecoderState*> VDS;
typedef VDS::const_iterator VDSit;
typedef Uint StateID;
typedef double MassePartielle;
typedef map<StateID, MassePartielle> MasseVu;
typedef boost::shared_ptr<MasseVu> MasseVu_ptr;


static void insertEscapes(string &str, const char *charsToEscape = "\"\\", char escapeChar = '\\')
{
   for ( string::size_type find = str.find_first_of(charsToEscape, 0); find != string::npos;
         find = str.find_first_of(charsToEscape, find + 2))
      {
         str.insert(find, 1, escapeChar);
      } // for
} // insertEscapes


PrintFunc::PrintFunc(vector<bool>* oovs) : oovs(oovs) {
  clear();
}

void
PrintFunc::clear() {
  sout.str("");
}

void
PrintFunc::appendPhrase(const DecoderState *state, PhraseDecoderModel &model) {
  string result;
  if (state != NULL) {
     model.getStringPhrase(result, state->trans->lastPhrase->phrase);
     sout << result;
  }
}

void
PrintFunc::appendQuotedPhrase(const DecoderState *state, PhraseDecoderModel &model) {
  if (state != NULL) {
    PhraseInfo *phrase = state->trans->lastPhrase;
    string p;
    model.getStringPhrase(p, phrase->phrase);
    insertEscapes(p);  // Escape all occurrences of " and \ in p
    sout << '"' << p << '"';
  }
}

void
PrintFunc::appendAlignmentInfo(const DecoderState *state, PhraseDecoderModel &model) {
  if (state != NULL) {
    PhraseInfo *phrase = state->trans->lastPhrase;
    bool oov = true;
    for (Uint i = phrase->src_words.start; i < phrase->src_words.end; ++i)
       if (!oovs || !(*oovs)[i]) {oov = false; break;}
    sout << "a=["
         << exp(phrase->phrase_trans_prob)      << ';'
         << phrase->src_words.start             << ';'
         << (phrase->src_words.end - 1)         << ';'
         << (oov ? "O" : "I")                << ']';
  }
}

void
PrintFunc::appendFFValues(const DecoderState *state, BasicModel &model) {
  if (state != NULL) {
    vector<double> ffvals;
    model.getFeatureFunctionVals(ffvals, *state->trans);
    assert(ffvals.size() > 0);

    sout << "v=[";
    for (Uint i = 0; i < ffvals.size(); i++) {
      if (i) sout << ';';
      sout << ffvals[i];
    }
    sout << ']';
  }
}

PrintPhraseOnly::PrintPhraseOnly(PhraseDecoderModel &m, vector<bool>* oovs): PrintFunc(oovs), model(m) {}

string PrintPhraseOnly::operator()(const DecoderState *state)
{
  clear();
  appendPhrase(state, model);
  return sout.str();
} // operator()

PrintTrace::PrintTrace(PhraseDecoderModel &m, vector<bool>* oovs): PrintFunc(oovs), model(m) {}

string PrintTrace::operator()(const DecoderState *state) {
  clear();
  if (state != NULL) {
    appendQuotedPhrase(state, model);
    sout << ' ';
    appendAlignmentInfo(state, model);
  }
  return sout.str();
}

PrintFFVals::PrintFFVals(BasicModel &m, vector<bool>* oovs): PrintFunc(oovs), model(m) {}

string PrintFFVals::operator()(const DecoderState *state) {
  clear();
  if (state != NULL) {
    appendQuotedPhrase(state, model);
    sout << ' ';
    appendFFValues(state, model);
  }
  return sout.str();
}

PrintAll::PrintAll(BasicModel &m, vector<bool>* oovs): PrintFunc(oovs), model(m) {}

string PrintAll::operator()(const DecoderState *state) {
  clear();
  if (state != NULL) {
    appendQuotedPhrase(state, model);
    sout << ' ';
    appendAlignmentInfo(state, model);
    sout << ' ';
    appendFFValues(state, model);
  }
  return sout.str();
}


namespace Portage
{
/**
 * Outputs the given transition in the following format:
 * (from (to "phrase" score))  if !backwards,
 * (to (from "phrase" score))  if backwards.
 * @param out       The stream to output to.
 * @param from      The first state.
 * @param to        The second state.
 * @param phrase    The phrase associated with the transition.
 * @param score     The score associated with the transition.
 * @param backwards Whether to reverse the direction of the transition.
 */
template<class T, class S>
static void printTransition(ostream *out, const T& from, const S& to, const string
                            &phrase, double score, bool backwards)
{
   if (!out) return;
   
   if (backwards)
      {
         printTransition(out, to, from, phrase, score, false);
      }
   else
      {
         // Escape all occurrences of " and \ in phrase
         string p = phrase;
         insertEscapes(p);

         *out << '(' << from << " (" << to << " \"" << p << "\" " << score << "))"
             << '\n';
      } // if
} // printTransition

/**
 * Outputs the coverage vector for the given state.  Specifically, the output
 * is the id of the state, followed by a string of 0's and 1's (the coverage
 * vector).  If the k-th character is a 1, this indicates that the k-th word in
 * the source sentence is covered by the given state; otherwise, the k-th word
 * is not covered.  Note: the computation here makes use of the condition that
 * the ranges in the sourceWordsNotCovered vector are in ascending order.
 * @param out   The stream te output to.
 * @param state A pointer to the state to output information for.
 */
static void printCoverageVector(ostream &out, const DecoderState *state)
{
   out << state->id << ' ';
   // Assume that src_words is in order
   Uint i = 0;
   Uint j = 0;
   UintSet::const_iterator it = state->trans->sourceWordsNotCovered.begin();
   while (j < state->trans->numSourceWordsCovered || it !=
          state->trans->sourceWordsNotCovered.end())
      {
         if (it == state->trans->sourceWordsNotCovered.end() || i < it->start)
            {
               out << '1';
               i++;
               j++;
            }
         else if (i == it->end)
            {
               it++;
            }
         else
            {
               out << '0';
               i++;
            } // if
      } // for
   out << '\n';
} // printCoverageVector

/**
 * Outputs all the transitions for the given state (ie. ones from state to
 * state->back and ones from state to recombined states of state->back).
 * @param out       The stream to output to.
 * @param covout    The stream to output coverage vector information to.
 * @param print     Print function that will format the state info to the proper format
 * @param state     A pointer to the DecoderState to output transitions for.
 * @param backwards Whether to output transitions in reverse.
 */
template<class PrintFunc>
static void doState(ostream *out, ostream *covout, PrintFunc &print,
                    const DecoderState *state, bool backwards)
{
   if (covout)
      printCoverageVector(*covout, state);

   if (state->back != NULL) {

      // Output the main transition
      double arcwt = exp(state->score - state->back->score);
      printTransition(out, state->back->id, state->id, print(state), arcwt, backwards);

      // Output all the transitions due to recombined translations
      for (vector<DecoderState *>::const_iterator it = state->back->recomb.begin();
           it < state->back->recomb.end(); it++) {
         // We take the difference in score between state and state->back
         // instead of between state and this recombined one (*it), because the
         // score on state was initially computed using state->back.
         printTransition(out, (*it)->id, state->id, print(state), arcwt, backwards);
      }
   }
} // doState

/**
 * Adds a given state to the state map if not already there.  If it is added,
 * then all the states with transitions to the given state are also added and
 * outputted, via a post-order traversal.
 * @param out       The stream to output to.
 * @param covout    The stream to output coverage vector information to.
 * @param print     Print function that will format the state info to the proper format.
 * @param stateMap  The map used to remember which states have already been
 *                  visited.
 * @param state     A pointer to the DecoderState to do.
 * @param backwards Whether to output transitions in reverse.
 * @param mv  Keeps track of the visited states and their sub lattice weight (dynamic programming)
 */
template<class PrintFunc>
static double addAndDoState(ostream *out, ostream *covout, PrintFunc &print,
                          SEEN &stateMap, DecoderState *state,
                          bool backwards, MasseVu_ptr mv)
{
   if (!stateMap[state->id]) {
   
      double dMasse(0.0);
      if (state->back != NULL) {
         const double dDelta(exp(state->score - state->back->score));
         dMasse = dDelta * addAndDoState(out, covout, print, stateMap, state->back, backwards, mv);
      }
      else {
         dMasse = 1.0f;
      }

      stateMap[state->id] = true;
      if (out) {
         doState(out, covout, print, state, backwards);
      }

      for ( VDSit it(state->recomb.begin()); it != state->recomb.end(); ++it) {
         assert(*it != NULL);
         assert((*it)->recomb.size() == 0);
         dMasse += addAndDoState(out, covout, print, stateMap, *it, backwards, mv);
      }

      // If we need to calculate Masse => keep a copy of Masse
      if (mv) (*mv)[state->id] = dMasse;
   }

   // We want the Masse return it or else 0.0f
   return ((mv) ? (*mv)[state->id] : 0.0f);
}


Uint maxID(const VDS& vds)
{
   Uint maxid(0);
   for (VDSit it1(vds.begin()); it1!=vds.end(); ++it1) {
      maxid = std::max(maxid, (*it1)->id);
      for (VDSit it2((*it1)->recomb.begin()); it2!=(*it1)->recomb.end(); ++it2) {
         maxid = std::max(maxid, (*it2)->id);
      }
   }
   return maxid;
}

/// Callable entity to fake deletion of pointers
template<typename T>
struct null_deleter
{
   /// Fakes deletion of a pointer.
   void operator()(T const *) const {}
};
   
double writeWordGraph(ostream *out, ostream *covout, PrintFunc &print,
                    const vector<DecoderState *> &finalStates, bool backwards, const bool bMasse)
{
   assert(!finalStates.empty());
   double dMasseTotale(0.0f);
   SEEN stateMap(maxID(finalStates)+1);
   // inline factory for MasseVu
   MasseVu_ptr mv(bMasse ? new MasseVu : static_cast<MasseVu*>(0), null_deleter<MasseVu>());
   if (out) {
      if (backwards) {
         *out << "0\n";
      } else {
         *out << "FINAL\n";
      }
   }
   // Post-order traversal through the word graph, beginning with each of the
   // final states
   typedef vector<DecoderState *>::const_iterator dsIT;
   for (dsIT it = finalStates.begin(); it != finalStates.end(); ++it) {
      // For each final state, we need an epsilon-transition from it to the
      // "FINAL" state (since we can only have 1 final state).  We do this
      // instead of giving the same name to all the final states so that all
      // the state names correspond to the id's (which may be used in verbose
      // output).
      if (backwards) {
         // If we are going backwards, then the "FINAL" state is actually the
         // initial state, so we need to output its transition first.
         printTransition(out, (*it)->id, "FINAL", print(NULL), 1, backwards);
         // WARNING WARNING we were unsure of where to put exactly the next for
         // loop sinc we are in backward mode, what is the reaction of lattice
         // processing software if the for loop is out of sequence. Since this
         // is almost unused and will get less and less used we feel that this
         // is the proper place for the for loop
         for (dsIT it2((*it)->recomb.begin()); it2 != (*it)->recomb.end(); ++it2)
         {
            printTransition(out, (*it2)->id, "FINAL", print(NULL), 1, backwards);
         }
         dMasseTotale += addAndDoState(out, covout, print, stateMap, (*it), backwards, mv);
      } else {
         // On the other hand, if we are going forward then since
         // addAndDoState() does a post-order traversal, the first transition
         // outputted will be from state 0, the initial state.
         dMasseTotale += addAndDoState(out, covout, print, stateMap, (*it), backwards, mv);
         printTransition(out, (*it)->id, "FINAL", print(NULL), 1, backwards);
         for (dsIT it2((*it)->recomb.begin()); it2 != (*it)->recomb.end(); ++it2)
         {
            printTransition(out, (*it2)->id, "FINAL", print(NULL), 1, backwards);
         }
      } // if
   } // for
   
   return dMasseTotale;
} // writeWordGraph

} // Portage
