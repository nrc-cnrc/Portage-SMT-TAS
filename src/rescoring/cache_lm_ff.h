/**
 * @author George Foster
 * @file cache_lm_ff.h   Cache LM feature
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l.information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef CACHE_LM_FF_H
#define CACHE_LM_FF_H

#include "featurefunction.h"
#include "docid.h"
#include "vocab_filter.h"

namespace Portage
{

class CacheLM: public FeatureFunction
{
   DocID docids;

   vector< vector<Uint> > freqs; // doc,voc-index -> freq of word in doc
   vector<Uint> total_freqs;    // doc -> sum of freqs of all words in doc

   Uint curr_doc;
   double curr_log_total_freq;

protected:   
   virtual bool loadModelsImpl() {return true;}

public:

   CacheLM(const string &args);
   virtual ~CacheLM();

   virtual Uint requires() { return FF_NEEDS_TGT_TOKENS | FF_NEEDS_TGT_VOCAB; }
   virtual FF_COMPLEXITY cost() const { return LOW; }

   virtual void preprocess(VocabFilter* tgt_vocab, Uint src_index, const Nbest& nb);


   virtual void init(const Sentences * const src_sents) {
      FeatureFunction::init(src_sents);
      if (src_sents->size() != docids.numSrcLines())
         error(ETFatal, "docids contents don't match number of source sentences");
   }

   virtual void source(Uint s, const Nbest * const nbest) {
      FeatureFunction::source(s, nbest);
      assert(s < docids.numSrcLines());
      curr_doc = docids.docID(s);
      curr_log_total_freq = log(double(total_freqs[curr_doc]));
   }

   virtual double value(Uint k) {
      double logprob = 0.0;
      const vector<string>& tgt = (*nbest)[k].getTokens();
      for (vector<string>::const_iterator itr=tgt.begin(); itr!=tgt.end(); ++itr)
         logprob += log(double(freqs[curr_doc][tgt_vocab->index(itr->c_str())])) - curr_log_total_freq;
      return logprob;
   }
};

}

#endif // CACHE_LM_FF_H
