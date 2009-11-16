/**
 * @author Michel Simard
 * @file levenshtein_ff.cc  Implementation for the Levenshtein feature function
 * (LevenshteinFF).
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/

#include "levenshtein_ff.h"
#include <str_utils.h>

using namespace Portage;

LevenshteinFF::LevenshteinFF(const string &args)
  : FeatureFunction(args)
{}

bool LevenshteinFF::parseAndCheckArgs() 
{
   if (argument.empty()) {
      error(ETWarn, "LevenshteinFF: You must provide a reference translation file");
      return false;
   }

   ref_file = argument;
   return true;
}

bool LevenshteinFF::loadModelsImpl()
{
  readFileLines(ref_file, ref);

  return true;
}

LevenshteinFF::~LevenshteinFF()
{
}

void
LevenshteinFF::source(Uint s, const Nbest * const nbest) {
   FeatureFunction::source(s, nbest);

   // Construct ngram set for given reference
   if (s >= ref.size()) {
     error(ETFatal, "Levenshtein: not enough references!");
     return;
   }
   // Tokenize the current reference translation
   ref_tokens.clear();
   split(ref[s], ref_tokens);
}


double 
LevenshteinFF::value(Uint k) {
  Tokens tgt_tokens = (*nbest)[k].getTokens();
  
  levenshtein.clear();
  return (double)levenshtein.LevenDist(tgt_tokens, ref_tokens);
}
