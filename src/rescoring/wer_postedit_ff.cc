/**
 * @author Nicola Ueffing
 * $Id$
 * @file wer_postedit_ff.cc
 * 
 * N-best Rescoring Module
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada 
 * 
 * Contains the implementation of the WER feature function for statistical post-editing.
 * Note that this is only useful for automatic post-editing in which source and target 
 * language are the same (well, maybe also for Catalan -> Castilian translation ;-)
 *
 * Note that the calculation could be speeded up by exploring the fact that a lot
 * of the sentences in the N-best list are the same or very similar. This has not
 * been done yet because this feature is used only in very rare cases.
 */

#include <fstream>
#include "wer_postedit_ff.h"

using namespace Portage;

WerPostedit::WerPostedit(const string& args) 
: FeatureFunction(args)
{ }

bool WerPostedit::parseAndCheckArgs()
{
   if (!argument.empty())
      error(ETWarn, "WerPostedit doesn't require arguments");
   return true;
}

void WerPostedit::source(Uint s, const Nbest * const nbest) {
   FeatureFunction::source(s, nbest);
   Tokens srcsent = (*src_sents)[s].getTokens();
   src.clear();
   src.push_back(srcsent);
   srclen = float(srcsent.size());
}

double
WerPostedit::computeValue(const Translation& trans) {
  if (trans.getTokens().size()==0)
    return 0;
  Uint dist = find_mWER(trans.getTokens(), src[0]);
  return dist/srclen;
}

