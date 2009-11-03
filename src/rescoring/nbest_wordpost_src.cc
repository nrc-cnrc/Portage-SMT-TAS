/**
 * @author Nicola Ueffing
 * @file nbest_wordpost_src.cc
 *
 *
 * COMMENTS: derived class for calculating word posterior probabilities over N-best lists
 * based on the source phrase to which the target word is aligned
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include <set>
#include "nbest_wordpost_src.h"

using namespace Portage;

void NBestWordPostSrc::setAlig(Alignment &al) {
   alig = al;
}


/**
 * Compute word posterior probabilities for all target words occurring in the N-best list
 */
void NBestWordPostSrc::computePosterior(Uint src_sent_id) {

   WordSrcPhrase2Posterior.clear();
   NBestPosterior::computePosterior(src_sent_id);

   for (uint n=0; n<min(Nbasis,nbest.size()); n++) {

      totalProb.update(scores[n],n);

      const Tokens&     hypn  = nbest[n].getTokens();
      Alignment         align = *(nbest[n].alignment);
      align.sortOnTarget();

      for (uint k=0; k<align.size(); k++) {
         const PhraseRange& trgp = align[k].target;
         set<Token> trgwords;
         for (uint i=trgp.first; i<=trgp.last; i++)
            trgwords.insert(hypn[i]);

         for (set<Token>::const_iterator titr=trgwords.begin(); titr!=trgwords.end(); titr++) {

            pair<Token,PhraseRange> p(*titr,align[k].source);

            if (WordSrcPhrase2Posterior.find(p) != WordSrcPhrase2Posterior.end())
               WordSrcPhrase2Posterior[p].update(scores[n],n);
            else
               WordSrcPhrase2Posterior[p] = ConfScore(scores[n],n,1);
         } // for titr
      } // for k
   } // for n

   /*
      for (map< pair<Token,PhraseRange>, ConfScore, wordSrcPhraseLessThan>::const_iterator itr=WordSrcPhrase2Posterior.begin(); itr!=WordSrcPhrase2Posterior.end(); itr++)
      cerr << itr->first.first << " aligned to src pos " << itr->first.second.first << "-" << itr->first.second.last << " : rel.freq " << itr->second.relfreq() << " : rank sum " << itr->second.rank() << " : prob " << itr->second.prob() << endl;
    */

   normalizePosterior();
}


/**
 * Normalize word posterior probabilities for all target words occurring in the N-best list
 */
void NBestWordPostSrc::normalizePosterior() {
   typedef map< pair<Token,PhraseRange>, ConfScore, wordSrcPhraseLessThan >::iterator ITERATOR;
   for (ITERATOR itr=WordSrcPhrase2Posterior.begin(); itr!=WordSrcPhrase2Posterior.end(); itr++)
      itr->second.normalize(totalProb);
}


/**
 * Determine the value for the whole sentence from word posterior probabilities
 * for given sentence 'trg'
 * -> values for whole sentence which can be used for rescoring
 */
double NBestWordPostSrc::sentPosteriorOne() {

   assert(WordSrcPhrase2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(alig.size());

   for (uint k=0; k<alig.size(); k++) {
      const PhraseRange& trgp = alig[k].target;
      for (uint i=trgp.first; i<=trgp.last; i++)
         conf.push_back(WordSrcPhrase2Posterior[make_pair(trg[i],alig[k].source)]);
   } // for k
   return sentPosteriorScores(conf);
}


/**
 * Determine and return all word posterior probabilities
 * for given sentence 'trg'
 */
vector<double> NBestWordPostSrc::wordPosteriorsOne() {

   assert(WordSrcPhrase2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(alig.size());

   for (uint k=0; k<alig.size(); k++) {
      const PhraseRange& trgp = alig[k].target;
      for (uint i=trgp.first; i<=trgp.last; i++)
         conf.push_back(WordSrcPhrase2Posterior[make_pair(trg[i],alig[k].source)]);
   } // for k
   return wordPosteriorProbs(conf);
}

/**
 * Output the word posterior probabilities for all words in the given hypothesis 'trg'
 */
void NBestWordPostSrc::tagPosteriorOne(ostream &out, int format) {

   assert(WordSrcPhrase2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(alig.size());

   for (uint k=0; k<alig.size(); k++) {
      const PhraseRange& trgp = alig[k].target;
      for (uint i=trgp.first; i<=trgp.last; i++)
         conf.push_back(WordSrcPhrase2Posterior[make_pair(trg[i],alig[k].source)]);
   } // for k

   printPosteriorScores(out,conf,trg,format);
}

/**
 * Output the word posterior probabilities for all words of all hypotheses in 'nbest'
 */
void NBestWordPostSrc::tagPosteriorAll(ostream &out, int format) {

   assert(WordSrcPhrase2Posterior.size());

   for (uint n=0; n<min(N,Ntag); n++) {
      trg  =   nbest[n].getTokens();
      alig = *(nbest[n].alignment);

      tagPosteriorOne(out,format);
   }
}

/**
 * Determine the value for the whole sentence from word posterior probabilities
 * for each sentence in nbest
 * -> values for whole sentence which can be used for rescoring
 */
void NBestWordPostSrc::tagSentPosteriorAll(ostream &out) {

   assert(WordSrcPhrase2Posterior.size());

   vector<ConfScore> conf;
   for (uint n=0; n<min(N,Ntag); n++) {

      trg  =   nbest[n].getTokens();
      alig = (*nbest[n].alignment);
      conf.clear();
      conf.reserve(alig.size());

      for (uint k=0; k<alig.size(); k++) {
         const PhraseRange& trgp = alig[k].target;
         for (uint i=trgp.first; i<=trgp.last; i++) {
            pair<Token,PhraseRange> p(trg[i],alig[k].source);
            assert(WordSrcPhrase2Posterior.find(p) != WordSrcPhrase2Posterior.end());
            conf.push_back(WordSrcPhrase2Posterior[p]);
         } // for i
      } // for k
      printSentPosteriorScores(out,conf);
   } // for n
}
