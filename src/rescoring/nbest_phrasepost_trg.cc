/**
 * @author Nicola Ueffing
 * @file nbest_phrasepost_trg.cc
 *
 *
 * COMMENTS: derived class for calculating phrase posterior probabilities over N-best lists
 * based on the fixed target position.
 * The terms 'posterior probability' and 'confidence value/measure' are used synonymously.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "nbest_phrasepost_trg.h"

using namespace Portage;

void NBestPhrasePostTrg::setAlig(Alignment &al) {
   alig = al;
}

/**
 * Compute phrase posterior probabilities for all target phrases occurring in the N-best list
 */ 
void NBestPhrasePostTrg::computePosterior(const Uint src_sent_id) {

   PhrasePos2Posterior.clear();
   NBestPosterior::computePosterior(src_sent_id);

   for (uint n=0; n<min(Nbasis,nbest.size()); n++) {

      totalProb.update(scores[n],n);

      const Tokens&     hypn = nbest[n].getTokens();
      Alignment        align = *(nbest[n].alignment);
      align.sortOnTarget();

      for (uint k=0; k<align.size(); k++) {

         Tokens trgp(hypn.begin() + align[k].target.first, hypn.begin() + align[k].target.last + 1);
         pair<Tokens,int> p(trgp,k);

         if (PhrasePos2Posterior.find(p) != PhrasePos2Posterior.end()) 
            PhrasePos2Posterior[p].update(scores[n],n);
         else 
            PhrasePos2Posterior[p] = ConfScore(scores[n],n,1);
      } // for k
   } // for n

   /*
      for (map< pair<Tokens,int>, ConfScore, phrasePosLessThan >::const_iterator itr=PhrasePos2Posterior.begin(); itr!=PhrasePos2Posterior.end(); itr++)
      cerr << itr->first.first.size() << " trg words " << " in trg pos " << itr->first.second << " : rel.freq " << itr->second.relfreq() << " : rank sum " << itr->second.rank() << " : prob " << itr->second.prob() << endl;
    */

   normalizePosterior();
}


/**
 * Normalize phrase posterior probabilities for all target phrases occurring in the N-best list
 */ 
void NBestPhrasePostTrg::normalizePosterior() {
   for (map< pair<Tokens,int>, ConfScore, phrasePosLessThan >::iterator itr=PhrasePos2Posterior.begin(); itr!=PhrasePos2Posterior.end(); itr++) 
      itr->second.normalize(totalProb);
}

/**
 * Determine the value for the whole sentence from phrase confidences
 * for given sentence 'trg'
 * -> values for whole sentence which can be used for rescoring
 */
double NBestPhrasePostTrg::sentPosteriorOne() {

   assert(PhrasePos2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(alig.size());

   for (uint k=0; k<alig.size(); k++) {
      Tokens trgp(trg.begin() + alig[k].target.first, trg.begin() + alig[k].target.last + 1);
      pair<Tokens,int> p(trgp,k);
      assert(PhrasePos2Posterior.find(p) != PhrasePos2Posterior.end());
      conf.push_back(PhrasePos2Posterior[p]);
   } // for k
   return sentPosteriorScores(conf);
}

/**
 * Determine and return posterior probabilities for all target words
 * for given sentence 'trg'
 */
vector<double> NBestPhrasePostTrg::wordPosteriorsOne() {

   assert(PhrasePos2Posterior.size());

   vector<double> conf;
   conf.reserve(alig.size());

   for (uint k=0; k<alig.size(); k++) {
      Tokens trgp(trg.begin() + alig[k].target.first, trg.begin() + alig[k].target.last + 1);
      pair<Tokens,int> p(trgp,k);
      assert(PhrasePos2Posterior.find(p) != PhrasePos2Posterior.end());
      const double c = PhrasePos2Posterior[p].prob() / (double)(alig[k].target.last+1 - alig[k].target.first);
      for (uint i=alig[k].target.first; i<=alig[k].target.last; i++) 
         conf.push_back(c);
   } // for k
   assert(conf.size() == trg.size());
   return conf;
}

/**
 * Output the phrase posterior probabilities for all phrases in the given hypothesis 'trg'
 */
void NBestPhrasePostTrg::tagPosteriorOne(ostream &out, const int &format) {

   assert(PhrasePos2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(alig.size());

   for (uint k=0; k<alig.size(); k++) {
      Tokens trgp(trg.begin() + alig[k].target.first, trg.begin() + alig[k].target.last + 1);
      pair<Tokens,int> p(trgp,k);
      conf.push_back(PhrasePos2Posterior[p]);
   }
   printPosteriorScores(out,conf,trg,format);
}


/**
 * Output the phrase posterior probabilities for all phrases of all hypotheses in 'nbest'
 */
void NBestPhrasePostTrg::tagPosteriorAll(ostream &out, const int &format) {

   assert(PhrasePos2Posterior.size());

   for (uint n=0; n<min(N,Ntag); n++) {
      trg  =   nbest[n].getTokens();
      alig = *(nbest[n].alignment);

      tagPosteriorOne(out,format);
   }
}

/**
 * Determine the value for the whole sentence from phrase confidences
 * for each sentence in nbest
 * -> values for whole sentence which can be used for rescoring
 */
void NBestPhrasePostTrg::tagSentPosteriorAll(ostream &out) {

   assert(PhrasePos2Posterior.size());

   vector<ConfScore> conf;
   for (uint n=0; n<min(N,Ntag); n++) {

      trg  =   nbest[n].getTokens();
      alig = (*nbest[n].alignment);
      conf.clear();
      conf.reserve(alig.size());

      for (uint k=0; k<alig.size(); k++) {
         Tokens           trgp(trg.begin() + alig[k].target.first, trg.begin() + alig[k].target.last + 1);
         pair<Tokens,int> p(trgp,k);
         assert(PhrasePos2Posterior.find(p) != PhrasePos2Posterior.end());
         conf.push_back(PhrasePos2Posterior[p]);
      } // for i
      printSentPosteriorScores(out,conf);
   } // for n
}
