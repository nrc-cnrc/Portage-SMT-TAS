/**
 * @author George Foster
 * @file cache_lm_ff.cc   Cache LM feature
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "cache_lm_ff.h"

using namespace Portage;

// We load the doc_ids file here rather than in loadModels() because it's cheap

CacheLM::CacheLM(const string &args) : 
   FeatureFunction(args),
   docids(args),
   freqs(docids.numDocs()),
   total_freqs(docids.numDocs())
{}

CacheLM::~CacheLM()
{}

void CacheLM::preprocess(VocabFilter* tgt_vocab, Uint src_index, const Nbest& nb)
{
   //cerr << src_index;

   assert(src_index < docids.numSrcLines());
   Uint doc_id = docids.docID(src_index);
   freqs[doc_id].resize(tgt_vocab->size());

   //cerr << " init stuff";

   for (Nbest::const_iterator s = nb.begin(); s != nb.end(); ++s) {
      //cerr << ".";
      const vector<string>& toks = s->getTokens();
      for (vector<string>::const_iterator t = toks.begin(); t != toks.end(); ++t) {
         Uint ti = tgt_vocab->index(t->c_str());
         assert(ti < tgt_vocab->size());
         ++freqs[doc_id][ti];
      }
      total_freqs[doc_id] += toks.size();
   }
   //   cerr << " done" << endl;
}



