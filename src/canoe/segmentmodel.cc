/**
 * @author Michel Simard
 * @file segmentmodel.cc  Implementation of all Segmentation model related decoder features.
 * 
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <errors.h>
#include <math.h>
#include "segmentmodel.h"
#include <str_utils.h>
#include <fstream>
#include <iostream>

using namespace Portage;

/************************** SegmentModel ***************************************/

SegmentationModel* SegmentationModel::create(const string& name_and_arg, bool fail)
{ 
   SegmentationModel* m = NULL;

   // Separate the model name and argument, introduced by # if present.
   vector<string> arg_split;
   split(name_and_arg, arg_split, "#", 2);
   const string name(arg_split.empty() ? "" : arg_split[0]);
   const string arg(arg_split.size() < 2 ? "" : arg_split[1]);

   if (name == "count")
      m = new SegmentCount();
   else if (name == "bernoulli")
      m = new BernoulliSegmentationModel(arg);
   else if (name == "uniform")
      m = new BernoulliSegmentationModel("0.5");
   else if (fail)
      error(ETFatal, "unknown segmentation model: " + name);

   return m;
}

/************************** BernoulliSegmentationModel **********************/

BernoulliSegmentationModel::BernoulliSegmentationModel(const string &arg) {
  double Q;
  conv(arg, Q);
  boundary = log(max(Q, 1e-6));
  not_boundary = log(max(1-Q, 1e-6));
}

double BernoulliSegmentationModel::precomputeFutureScore(const PhraseInfo& phrase_info)
{
   int words = phrase_info.src_words.end - phrase_info.src_words.start;
   return boundary + (words - 1) * not_boundary;
}

