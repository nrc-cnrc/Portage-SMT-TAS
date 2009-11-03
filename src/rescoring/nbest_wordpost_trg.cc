/**
 * @author Nicola Ueffing
 * @file nbest_wordpost_trg.cc
 *
 *
 * COMMENTS: derived class for calculating word posterior probabilities over N-best lists
 * based on the fixed target position (simple method which serves as baseline only
 * The terms 'posterior probability' and 'confidence value/measure' are used synonymously.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "nbest_wordpost_trg.h"

using namespace Portage;

/**
 * Compute word posterior probabilities for all target words occurring in the N-best list
 */
void NBestWordPostTrg::computePosterior(Uint src_sent_id) {

   WordPos2Posterior.clear();
   NBestPosterior::computePosterior(src_sent_id);

   for (uint n=0; n<min(Nbasis,nbest.size()); n++) {

      totalProb.update(scores[n],n);

      const Tokens& hypn = nbest[n].getTokens();

      for (uint i=0; i<hypn.size(); i++) {
         pair<Token,int> p(hypn[i],i);
         if (WordPos2Posterior.find(p)!=WordPos2Posterior.end())
            WordPos2Posterior[p].update(scores[n],n);
         else
            WordPos2Posterior[p] = ConfScore(scores[n],n,1);
         //      cerr << hypn[i] << " " << scores[n] << " " << n << endl;
      } // for i
   } // for n

   /*
      for (map< pair<Token,int>, ConfScore >::const_iterator itr=WordPos2Posterior.begin(); itr!=WordPos2Posterior.end(); itr++)
      cerr << itr->first.first << " in pos " << itr->first.second << " : rel.freq " << itr->second.relfreq() << " , rank weighted freq. " << itr->second.rank() << " , prob " << itr->second.prob() << endl;
    */

   normalizePosterior();
}


/**
 * Normalize word posterior probabilities for all target words occurring in the N-best list
 */
void NBestWordPostTrg::normalizePosterior() {
   for (map< pair<Token,int>, ConfScore >::iterator itr=WordPos2Posterior.begin(); itr!=WordPos2Posterior.end(); itr++)
      itr->second.normalize(totalProb);
}

/**
 * Determine the value for the whole sentence from word confidences
 * for given sentence 'trg'
 * -> values for whole sentence which can be used for rescoring
 */
double NBestWordPostTrg::sentPosteriorOne() {

   assert(WordPos2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(trg.size());

   for (uint i=0; i<trg.size(); i++) {
      pair<Token,int> p(trg[i],i);
      assert(WordPos2Posterior.find(p) != WordPos2Posterior.end());
      conf.push_back(WordPos2Posterior[p]);
   } // for i
   return sentPosteriorScores(conf);
}

/**
 * Determine and return all word posterior probabilities
 * for given sentence 'trg'
 */
vector<double> NBestWordPostTrg::wordPosteriorsOne() {

   assert(WordPos2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(trg.size());

   for (uint i=0; i<trg.size(); i++) {
      pair<Token,int> p(trg[i],i);
      assert(WordPos2Posterior.find(p) != WordPos2Posterior.end());
      conf.push_back(WordPos2Posterior[p]);
   } // for i
   return wordPosteriorProbs(conf);
}
/**
 * Output the word posterior probabilities for all words in the given hypothesis 'trg'
 */
void NBestWordPostTrg::tagPosteriorOne(ostream &out, int format) {

   assert(WordPos2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(trg.size());

   for (uint i=0; i<trg.size(); i++)
      conf.push_back(WordPos2Posterior[make_pair(trg[i],i)]);

   printPosteriorScores(out,conf,trg,format);
}


/**
 * Output the word posterior probabilities for all words of all hypotheses in 'nbest'
 */
void NBestWordPostTrg::tagPosteriorAll(ostream &out, int format) {

   assert(WordPos2Posterior.size());

   for (uint n=0; n<min(N,Ntag); n++) {
      trg = nbest[n].getTokens();
      tagPosteriorOne(out,format);
   }
}

/**
 * Determine the value for the whole sentence from word confidences
 * for each sentence in nbest
 * -> values for whole sentence which can be used for rescoring
 */
void NBestWordPostTrg::tagSentPosteriorAll(ostream &out) {

   assert(WordPos2Posterior.size());

   vector<ConfScore> conf;
   for (uint n=0; n<min(N,Ntag); n++) {

      trg = nbest[n].getTokens();
      conf.clear();
      conf.reserve(trg.size());

      for (uint i=0; i<trg.size(); i++) {
         pair<Token,int> p(trg[i],i);
         assert(WordPos2Posterior.find(p) != WordPos2Posterior.end());
         conf.push_back(WordPos2Posterior[p]);
      } // for i
      printSentPosteriorScores(out,conf);
   } // for n
}
