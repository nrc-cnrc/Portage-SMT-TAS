/**
 * @author Nicola Ueffing
 * @file nbest_sentlenpost.cc
 *
 *
 * COMMENTS: derived class for calculating sentence length posterior probabilities
 * over N-best lists.
 * The terms 'posterior probability' and 'confidence value/measure' are used synonymously.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "nbest_sentlenpost.h"

using namespace Portage;

/**
 * Compute sentence length posterior probabilities for all sentence lengths
 * occurring in the N-best list
 */
void NBestSentLenPost::computePosterior(Uint src_sent_id) {

   Len2Posterior.clear();
   NBestPosterior::computePosterior(src_sent_id);

   for (uint n=0; n<min(Nbasis,nbest.size()); n++) {

      totalProb.update(scores[n],n);

      const int hyplen = nbest[n].getTokens().size();

      if (Len2Posterior.find(hyplen) != Len2Posterior.end())
         Len2Posterior[hyplen].update(scores[n],n);
      else
         Len2Posterior[hyplen] = ConfScore(scores[n],n,1);

   } // for n

   /*
      for (map<int, ConfScore>::const_iterator itr=Len2Posterior.begin(); itr!=Len2Posterior.end(); itr++)
      cerr << itr->first << " trg words : rel.freq " << itr->second.relfreq() << " : rank sum " << itr->second.rank() << " : prob " << itr->second.prob() << endl;
    */

   normalizePosterior();
}


/**
 * Normalize sentence length posterior probabilities for all target sentence lengths
 * occurring in the N-best list
 */
void NBestSentLenPost::normalizePosterior() {
   for (map<int, ConfScore>::iterator itr=Len2Posterior.begin(); itr!=Len2Posterior.end(); itr++)
      itr->second.normalize(totalProb);
}

/**
 * Determine the value for the given sentence 'trg'
 * -> can be used for rescoring
 */
double NBestSentLenPost::sentPosteriorOne() {

   assert(Len2Posterior.size());

   const vector<ConfScore> conf(1,Len2Posterior[trg.size()]);
   assert(conf.size() == 1);

   return sentPosteriorScores(conf);
}

/**
 * Determine and return all word posterior probabilities
 * for given sentence 'trg'
 */
vector<double> NBestSentLenPost::wordPosteriorsOne() {

   assert(Len2Posterior.size());

   const double c = Len2Posterior[trg.size()].prob() / (double)trg.size();
   const vector<double> conf(trg.size(), c);
   assert(conf.size() == trg.size());

   return conf;
}

/**
 * Output the sentence length posterior probabilities for the given hypothesis 'trg'
 */
void NBestSentLenPost::tagPosteriorOne(ostream &out, int format) {

   assert(Len2Posterior.size());

   const vector<ConfScore> conf(1, Len2Posterior[trg.size()]);
   assert(conf.size() == 1);

   printPosteriorScores(out,conf,trg,format);
}


/**
 * Output the sentence length posterior probabilities for all hypotheses in 'nbest'
 */
void NBestSentLenPost::tagPosteriorAll(ostream &out, int format) {

   assert(Len2Posterior.size());

   for (uint n=0; n<min(N,Ntag); n++) {
      trg = nbest[n].getTokens();
      tagPosteriorOne(out,format);
   }
}

/**
 * Determine the value for each sentence in nbest
 * -> can be used for rescoring
 */
void NBestSentLenPost::tagSentPosteriorAll(ostream &out) {

   assert(Len2Posterior.size());

   for (uint n=0; n<min(N,Ntag); n++) {

      trg = nbest[n].getTokens();
      const vector<ConfScore> conf(1, Len2Posterior[trg.size()]);
      assert(conf.size() == 1);
      printSentPosteriorScores(out,conf);

   } // for n
}
