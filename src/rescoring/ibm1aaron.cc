/**
 * @author Aaron Tikuisis
 * @file ibm1aaron.cc  Implementation of IBM1AaronTgtGivenSrc.
 *
 * $Id$
 * 
 * K-best Rescoring Module
 * 
 * Contains the implementation of IBM1AaronTgtGivenSrc, which is a feature function
 * defined as follows:
 * \f$ \max_{f:\mbox{src\_toks} \rightarrow \mbox{tgt\_toks} (1-1)} \sum_{s \in \mbox{src\_toks}}
 *	( \frac{P(f(s)|s)}{max_{t \in \mbox{tgt\_vocab}} P(t|s)} ) \f$
 * 
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Conseil national de recherches du Canada / Copyright 2004, National Research Council of Canada 
 */

#include "featurefunction.h"
#include "ibm1aaron.h"
#include "fmax.h"
#include <ttablewithmax.h>
#include <vector>
#include <string>

using namespace std;
using namespace Portage;

IBM1AaronBase::IBM1AaronBase(const string &file): table(file) {}

double 
IBM1AaronBase::computeValue(const Tokens& src, const Tokens& tgt) {
  double probs[src.size()][tgt.size()];
  double *probsP[src.size()];

  for (Uint i = 0; i < src.size(); i++) {
    probsP[i] = probs[i];
    double curMax = table.maxSourceProb(src[i]);
    if (curMax == 0) {
      // Clearly, not in the table at all, but we want still to avoid dividing by 0.
      curMax = 1;
    } // if
    for (Uint j = 0; j < tgt.size(); j++) {
      probsP[i][j] = max(table.getProb(src[i], tgt[j]), 0) / curMax;
      //cerr << probsP[i][j] << '\t';
    } // for
      //cerr << endl;
  } // for

  return max_1to1_func(probsP, src.size(), tgt.size());
} // IBM1WTransTgtGivenSrc
