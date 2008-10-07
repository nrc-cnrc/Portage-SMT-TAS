/**
 * @author Nicola Ueffing
 * $Id$
 * @file bleu_postedit_ff.cc
 * 
 * N-best Rescoring Module
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada 
 * 
 * Contains the implementation of the BLEU feature function for statistical post-editing.
 * Note that this is only useful for automatic post-editing in which source and target 
 * language are the same (well, maybe also for Catalan -> Castilian translation ;-)
 */

#include <fstream>
#include "bleu_postedit_ff.h"

using namespace Portage;

BleuPostedit::BleuPostedit(const string& args) 
: FeatureFunction(args)
{ }

bool BleuPostedit::parseAndCheckArgs()
{
   // Smoothing type
  smoothbleu = atoi(argument.c_str());
   return true;
}

void BleuPostedit::source(Uint s, const Nbest * const nbest) {
   FeatureFunction::source(s, nbest);
   Tokens srcsent = (*src_sents)[s].getTokens();
   src.clear();
   src.push_back(srcsent);
}

double
BleuPostedit::computeValue(const Translation& trans) {
  bleustats.init(trans.getTokens(), src, smoothbleu);
  if (trans.getTokens().size()==0)
    return 0;
  return bleustats.score();
}

