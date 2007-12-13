/**
 * @author George Foster
 * @file ibm_feature.cc  IBM-based feature functions for canoe.
 * 
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "basicmodel.h"
#include "ibm_feature.h"

using namespace Portage;

IBM1FwdFeature::IBM1FwdFeature(BasicModelGenerator* bmg, const string& modelfile) :
   bmg(bmg)
{
   cerr << "loading IBM1 model from " << modelfile <<  " ... " << flush;
   time_t start_time = time(NULL);
   ibm1 = new IBM1(modelfile, bmg->limitPhrases ? &(bmg->get_voc()) : NULL);
   cerr << "done in " << (time(NULL) - start_time) << "s" << endl;
}

void IBM1FwdFeature::newSrcSent(const vector<string>& src_sent,
                                vector<PhraseInfo *>** phrase_infos) 
{
   // set active voc
   active_voc.clear();
   for (Uint i = 0; i < src_sent.size(); ++i)
      for (Uint j = 0; j < src_sent.size()-i; ++j)
         for (Uint k = 0; k < phrase_infos[i][j].size(); ++k) {
            //for (Uint l = 0; l < phrase_infos[i][j][k]->phrase.size(); ++l)
            //   active_voc.insert(phrase_infos[i][j][k]->phrase[l]);
            Phrase &phrase(phrase_infos[i][j][k]->phrase);
            for ( Phrase::const_iterator w_it(phrase.begin());
                  w_it != phrase.end(); ++w_it )
               active_voc.insert(*w_it);
         }

   // assign logprobs to words in active voc
   //    double pr(const vector<string>& src_toks, const string& tgt_tok);
   string word;
   logprobs.resize(active_voc.size());
   for (Uint i = 0; i < active_voc.size(); ++i) {
      bmg->getTargetWord(active_voc.contents()[i], word);
      double pr = ibm1->pr(src_sent, word);
      logprobs[i] = pr == 0 ? LOG_ALMOST_ZERO : log(pr);
// cerr << word << ": " << exp(logprobs[i]) << endl;
   }
}

double IBM1FwdFeature::phraseLogProb(const Phrase& phrase)
{
   double s = 0.0;
   for ( Phrase::const_iterator w_it(phrase.begin());
         w_it != phrase.end(); ++w_it ) {
      Uint wi = active_voc.find(*w_it);
      assert(wi < active_voc.size());
      s += logprobs[wi];
   }
   return s;
}

