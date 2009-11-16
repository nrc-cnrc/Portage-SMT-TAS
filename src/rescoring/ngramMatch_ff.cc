/**
 * @author Michel Simard
 * @file ngramMatch_ff.cc  Implementation for the ngramMatch feature function
 * (NGramMatchFF).
 *
 * $Id$ 
 *
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
*/

#include "ngramMatch_ff.h"
#include <str_utils.h>

using namespace Portage;

NGramMatchFF::NGramMatchFF(const string &args)
  : FeatureFunction(args), ngram_size(0), ref_ngrams()
{}

bool NGramMatchFF::parseAndCheckArgs() 
{
   if (argument.empty()) {
      error(ETWarn, "NGramMatchFF: You must provide a reference translation file and ngram size");
      return false;
   }

   vector<string> args;
   if (split(argument, args, "#") != 2) {
      error(ETWarn, "NGramMatchFF: You must provide a reference translation file and ngram size");
      return false;
   }
   ref_file = args[0];
   if (!conv(args[1], ngram_size) || ngram_size < 1) {
     error(ETWarn, "NGramMatchFF: 2nd arg ngram_size must be strictly positive integer");
     return false;
   }
   return true;
}

bool NGramMatchFF::loadModelsImpl()
{
  readFileLines(ref_file, ref);

  return true;
}

NGramMatchFF::~NGramMatchFF()
{
}

void
NGramMatchFF::source(Uint s, const Nbest * const nbest) {
   FeatureFunction::source(s, nbest);

   // Construct ngram set for given reference
   if (s >= ref.size()) {
     error(ETFatal, "NGramMatch: not enough references!");
     return;
   }
   ref_ngrams.clear();
   vector<string> ref_tokens;
   split(ref[s], ref_tokens);
   for (Uint i = 0; i + ngram_size <= ref_tokens.size(); ++i)
     ref_ngrams.insert(NGram<string>(ref_tokens, i, ngram_size));
}


double 
NGramMatchFF::value(Uint k) {
  Tokens tgt_tokens = (*nbest)[k].getTokens();

  NGramSet tgt_ngrams;
  for (Uint i = 0; i + ngram_size <= tgt_tokens.size(); ++i)
    tgt_ngrams.insert(NGram<string>(tgt_tokens, i, ngram_size));
  
  Uint count = 0;
  NGramSet::const_iterator x = ref_ngrams.begin();
  NGramSet::const_iterator y = tgt_ngrams.begin();
  while (x != ref_ngrams.end() && y != tgt_ngrams.end()) {
    if ((*x) == (*y)) {
      ++x; ++y; ++count;
    } else if ((*x) < (*y)) {
      ++x;
    } else {
      ++y;
    }
  }

  return (double)count;
}
