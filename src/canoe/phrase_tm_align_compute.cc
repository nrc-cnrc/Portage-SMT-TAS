/**
 * @author Aaron Tikuisis
 *	**Modified by Matthew Arnold for align,ent-training purposes
 *   **Modified by Nicola Ueffing to use all decoder models
 * @file phrase_tm_align_compute.cc  This file contains the code used to
 * compute the phrase-based translation probability.  Essentially, it uses the
 * beam-search decoder algorithm, with a model (ForcePhrasePDM) that constrains
 * the target sentence.
 *
 * $Id$
 *
 * Translation-Model Utilities
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

#include "phrase_tm_align.h"
#include <decoder.h>
#include <hypothesisstack.h>
#include <forced_phrase_finder.h>
#include <canoe_general.h>
#include <str_utils.h>
#include <errors.h>
#include "alignedphrase.h"
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

using namespace std;
using namespace Portage;

PhraseTMAligner::PhraseTMAligner(const CanoeConfig& c,
                                 const vector< vector<string> > &src_sents,
                                 const vector< vector<MarkedTranslation> > &marked_src_sents)
: verbosity(c.verbosity)
{
   gen = BasicModelGenerator::create(c, &src_sents, &marked_src_sents);
   if ( c.verbosity >= 1) {
      string weightstr;
      cerr << "Decoder features:" << endl << gen->describeModel()
           << "Feature weights: " << c.getFeatureWeightString(weightstr) << endl;
   }
} // PhraseTMAligner

PhraseTMAligner::PhraseTMAligner(const CanoeConfig& c)
: verbosity(c.verbosity)
{
   gen = BasicModelGenerator::create(c);
   if ( c.verbosity >= 1) {
      string weightstr;
      cerr << "Decoder features:" << endl << gen->describeModel()
           << "Feature weights: " << c.getFeatureWeightString(weightstr) << endl;
   }
} // PhraseTMAligner

void PhraseTMAligner::computePhraseTM(const vector<string> &src_sent,
      const vector<MarkedTranslation> &marked_src_sent,
      const vector<string> &tgt_sent, stringstream &ss,
      Uint n, bool noscore, bool onlyscore,
      double threshold, Uint pruneSize, Uint covLimit, double covThreshold)
{
   BasicModel *model = gen->createModel(src_sent, marked_src_sent, true, NULL);

   // Create the model/phrase finder to be used -- distortion parameters are
   // directly taken from model.c, and taken into account.
   ForcedTargetPhraseFinder finder(*model, tgt_sent);

    // Create the hypothesis stacks (do no pruning)
   HypothesisStack *stack[src_sent.size() + 1];
   for (Uint i = 0; i <= src_sent.size(); i++)
      stack[i] = new HistogramThresholdHypStack(*model, pruneSize, log(threshold),
                                                covLimit, log(covThreshold));

   // Use the decoder algorithm to maximize the TM score
   runDecoder(*model, stack, src_sent.size(), finder, verbosity);

   if (stack[src_sent.size()]->isEmpty())
      // The translation is precluded by the model
      for (Uint i = 0; i < n; i++)
         ss << '\n';

   else {

      // Find and return the score and distortion
      DecoderState *result = stack[src_sent.size()]->pop();

      int num = 1;

      alignList *final = makeAlignments(result, model, n, tgt_sent.size());
      while (!stack[src_sent.size()]->isEmpty()) {
         DecoderState *result = stack[src_sent.size()]->pop();

         alignList *list = makeAlignments(result, model, n, tgt_sent.size());
         final = mergeAligned(final, list, n);
         num++;
      }
      printAlignedList(ss, final, model, n, !noscore, onlyscore);

      delete final;
   } // if

   for (Uint i = 0; i <= src_sent.size(); i++)
      delete stack[i];

   delete model;
} // computePhraseTM

