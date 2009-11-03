/**
 * @author Nicola Ueffing
 * @file nbest_phrasepost_src.cc
 *
 *
 * COMMENTS: derived class for calculating phrase posterior probabilities over N-best lists
 * based on the aligned source phrase
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include <set>
#include "nbest_phrasepost_src.h"

using namespace Portage;

void NBestPhrasePostSrc::setAlig(Alignment &al) {
   alig = al;
}


/**
 * Compute phrase posterior probabilities for all target phrases occurring in the N-best list
 */
void NBestPhrasePostSrc::computePosterior(Uint src_sent_id) {

   PhrasePair2Posterior.clear();
   NBestPosterior::computePosterior(src_sent_id);

   for (uint n=0; n<min(Nbasis,nbest.size()); n++) {

      totalProb.update(scores[n],n);

      const Tokens&     hypn  = nbest[n].getTokens();
      Alignment         align = *(nbest[n].alignment);
      align.sortOnTarget();

      for (uint k=0; k<align.size(); k++) {

         Tokens trgp(hypn.begin() + align[k].target.first, hypn.begin() + align[k].target.last + 1);
         pair<Tokens,PhraseRange> p(trgp,align[k].source);

         if (PhrasePair2Posterior.find(p) != PhrasePair2Posterior.end())
            PhrasePair2Posterior[p].update(scores[n],n);
         else
            PhrasePair2Posterior[p] = ConfScore(scores[n],n,1);
      } // for k
   } // for n

   /*
      for (map< pair<Tokens,PhraseRange>, ConfScore, phrasePairLessThan>::const_iterator itr=PhrasePair2Posterior.begin(); itr!=PhrasePair2Posterior.end(); itr++)
      cerr << itr->first.first.size() << " trg words " << " aligned to src pos " << itr->first.second.first << "-" << itr->first.second.last << " : rel.freq " << itr->second.relfreq() << " : rank sum " << itr->second.rank() << " : prob " << itr->second.prob() << endl;
    */

   normalizePosterior();
}


/**
 * Normalize phrase posterior probabilities for all target phrases occurring in the N-best list
 */
void NBestPhrasePostSrc::normalizePosterior() {
   typedef map< pair<Tokens,PhraseRange>, ConfScore, phrasePairLessThan >::iterator ITERATOR;
   for (ITERATOR itr=PhrasePair2Posterior.begin(); itr!=PhrasePair2Posterior.end(); itr++)
      itr->second.normalize(totalProb);
}


/**
 * Determine the value for the whole sentence from phrase posterior probabilities
 * -> values for whole sentence which can be used for rescoring
 */
double NBestPhrasePostSrc::sentPosteriorOne() {

   assert(PhrasePair2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(alig.size());

   for (uint k=0; k<alig.size(); k++) {
      Tokens trgp(trg.begin() + alig[k].target.first, trg.begin() + alig[k].target.last + 1);
      conf.push_back(PhrasePair2Posterior[make_pair(trgp,alig[k].source)]);
   }
   return sentPosteriorScores(conf);
}


/**
 * Determine and return posterior probabilities for all target words
 * for given sentence 'trg'
 */
vector<double> NBestPhrasePostSrc::wordPosteriorsOne() {

   assert(PhrasePair2Posterior.size());

   vector<double> conf;

   for (uint k=0; k<alig.size(); k++) {
      Tokens trgp(trg.begin() + alig[k].target.first, trg.begin() + alig[k].target.last + 1);
      const double c = PhrasePair2Posterior[make_pair(trgp,alig[k].source)].prob() / (double)(alig[k].target.last+1 - alig[k].target.first);
      for (uint i=alig[k].target.first; i<=alig[k].target.last; i++)
         conf.push_back(c);
   } // for k
   assert(conf.size() == trg.size());
   return conf;
}

/**
 * Output the phrase posterior probabilities for all phrases in the target sentence
 */
void NBestPhrasePostSrc::tagPosteriorOne(ostream &out, int format) {

   assert(PhrasePair2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(alig.size());

   for (uint k=0; k<alig.size(); k++) {
      Tokens trgp(trg.begin() + alig[k].target.first, trg.begin() + alig[k].target.last + 1);
      conf.push_back(PhrasePair2Posterior[make_pair(trgp,alig[k].source)]);
   }
   trg.resize(conf.size(),0);

   printPosteriorScores(out,conf,trg,format);
}

/**
 * Output the phrase posterior probabilities for all phrases of all hypotheses in 'nbest'
 */
void NBestPhrasePostSrc::tagPosteriorAll(ostream &out, int format) {

   assert(PhrasePair2Posterior.size());

   for (uint n=0; n<min(N,Ntag); n++) {
      trg  =   nbest[n].getTokens();
      alig = *(nbest[n].alignment);

      tagPosteriorOne(out,format);
   }
}

/**
 * Determine the value for the whole sentence from phrase posteriors
 * for each sentence in nbest
 * -> values for whole sentence which can be used for rescoring
 */
void NBestPhrasePostSrc::tagSentPosteriorAll(ostream &out) {

   assert(PhrasePair2Posterior.size());

   vector<ConfScore> conf;
   for (uint n=0; n<min(N,Ntag); n++) {

      trg  =   nbest[n].getTokens();
      alig = (*nbest[n].alignment);
      conf.clear();
      conf.reserve(alig.size());

      for (uint k=0; k<alig.size(); k++) {
         Tokens trgp(trg.begin() + alig[k].target.first, trg.begin() + alig[k].target.last + 1);
         pair<Tokens,PhraseRange> p(trgp,alig[k].source);
         assert(PhrasePair2Posterior.find(p) != PhrasePair2Posterior.end());
         conf.push_back(PhrasePair2Posterior[p]);
      } // for k
      printSentPosteriorScores(out,conf);
   } // for n
}
