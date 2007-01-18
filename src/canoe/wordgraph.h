/**
 * @author Aaron Tikuisis
 * @file wordgraph.h  This file contains the declaration of the
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

#ifndef WORDGRAPH_H
#define WORDGRAPH_H

#include <math.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <stdio.h>


namespace Portage
{

using namespace std;

class DecoderState;
class PhraseDecoderModel;
class BasicModel;

/// Interface for Lattice printing functions
/// (lattice information extraction).
class PrintFunc {
 public:
  /// Constructor.
  /// 
  PrintFunc(vector<bool>* oovs = NULL);
  /// Destructor.
  virtual ~PrintFunc() {}

  /**
   * Gets the proper string from a state of a lattice.
   * Makes the class a callable entity on a state.
   * @param state  state from which to create the info string
   * @return Returns the information of a state
   */
  virtual string operator()(const DecoderState *state) = 0;

 protected:
   ostringstream sout;  ///< Buffer to construct the final output
   vector<bool>* oovs;  ///< source sentence OOVs

   /// Clears the sout to start a new output.
   void clear();
   /**
    * Appends the state's phrase to sout.
    * @param state  current state
    * @param model  model from which the state was created
    */
   void appendPhrase(const DecoderState *state, PhraseDecoderModel &model);
   /**
    * Appends the quoted state's phrase to sout.
    * @param state  current state
    * @param model  model from which the state was created
    */
   void appendQuotedPhrase(const DecoderState *state, PhraseDecoderModel &model);
   /**
    * Appends the state's phrase and OOV to sout.
    * @param state  current state
    * @param model  model from which the state was created
    */
   void appendAlignmentInfo(const DecoderState *state, PhraseDecoderModel &model);
   /**
    * Appends the state's phrase and Feature Function values to sout.
    * @param state  current state
    * @param model  model from which the state was created
    */
   void appendFFValues(const DecoderState *state, BasicModel &model);
}; // PrintFunc


//-----------------------------------------------------------------------------
/// Prints phrase only (lattice information extraction).
class PrintPhraseOnly: public PrintFunc {
  PhraseDecoderModel &model;  ///< Model which created the states
public:
   /// Constructor.
   /// @param model model from which the states were created 
   /// @param oovs  source sentence oov vector 
   PrintPhraseOnly(PhraseDecoderModel &model, vector<bool>* oovs = NULL);
   virtual string operator()(const DecoderState *state);
}; // PrintPhraseOnly


//-----------------------------------------------------------------------------
/// Prints phrase and alignments (lattice information extraction).
class PrintTrace: public PrintFunc {
  PhraseDecoderModel &model;  ///< Model which created the states
public:
   /// Constructor.
   /// @param model model from which the states were created 
   /// @param oovs  source sentence oov vector 
   PrintTrace(PhraseDecoderModel &model, vector<bool>* oovs = NULL);
   virtual string operator()(const DecoderState *state);
}; // PrintTrace


//-----------------------------------------------------------------------------
/// Prints phrase and the feature funtion values
/// (lattice information extraction).
class PrintFFVals: public PrintFunc {
  BasicModel &model;  ///< Model which created the states
public:
   /// Constructor.
   /// @param model model from which the states were created 
   /// @param oovs  source sentence oov vector 
   PrintFFVals(BasicModel &model, vector<bool>* oovs = NULL);
   virtual string operator()(const DecoderState *state);
}; // PrintFFVals


//-----------------------------------------------------------------------------
/// Prints phrase, alignment and feature function values
/// (lattice information extraction).
class PrintAll: public PrintFunc {
  BasicModel &model;  ///< Model which created the states
public:
   /// Constructor.
   /// @param model model from which the states were created 
   /// @param oovs  source sentence oov vector 
   PrintAll(BasicModel &model, vector<bool>* oovs = NULL);
   virtual string operator()(const DecoderState *state);
}; // PrintAll


/**
 * Outputs the probabilistic finite state machine for the given set of final
 * hypotheses, in a format readable by standard lattice processing packages.
 * Specifically, the format is as follows:
 * The first line contains the name of the final state.
 * Each subsequent line contains a transition in the following format:
 * (from (to "phrase" score))
 * The initial state is the "from" state in the first transition listed.  The
 * score is multiplicative, ie. the score of a sentence (path from initial to
 * final state) is the product of the scores of each transition.
 * @return              The weight of the lattice
 * @param out           The stream to output to (don't if null)
 * @param covout        The stream to output coverage vector information to
 *                      (don't if null).
 * @param print         The print function used to produce the "phrase" part of
 *                      the output.
 * @param finalStates   The vector containing all the final DecoderStates.
 * @param backwards     Whether to reverse the entire word graph (ie, flip the
 *                      initial and final states and reverse the direction of
 *                      each transition).
 * @param bMasse        Indicates if we should calculate the weight of the
 *                      lattice.
 */
double writeWordGraph(ostream* out, ostream* covout, PrintFunc &print,
                    const vector<DecoderState *> &finalStates,
                    bool backwards = false, const bool bMasse = false);


} // ends namespace Portage

#endif // WORDGRAPH_H
