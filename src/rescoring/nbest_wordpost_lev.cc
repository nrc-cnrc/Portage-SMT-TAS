/**
 * @author Nicola Ueffing
 * @file nbest_wordpost_lev.cc
 *
 *
 * COMMENTS: derived class for calculating word posterior probabilities over N-best lists
 * based on the Levenshtein-aligned target position (usually best method for WER-based classification)
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "nbest_wordpost_lev.h"
#include "levenshtein.h"

using namespace Portage;

void NBestWordPostLev::computePosterior(const Uint src_sent_id) {
   NBestPosterior::computePosterior(src_sent_id);
}

void NBestWordPostLev::init(const Tokens &t) {

   NBestPosterior::init(t);

   conf.clear();
   conf.resize(trg.size());

   Levenshtein<Token> leven;
   leven.setVerbosity(0);

   totalProb.reset();
   for (uint n=0; n<min(Nbasis,nbest.size()); n++) {

      totalProb.update(scores[n],n);

      const Tokens& hypn = nbest[n].getTokens();
      const vector<int> levAlig = leven.LevenAlig(trg,hypn);

      for (uint i=0; i<trg.size(); i++) 
         if (levAlig[i] > -1)
            if (hypn[levAlig[i]] == trg[i])
               conf[i].update(scores[n],n);

   } // for n
   normalizePosterior();
}

/**
 * Normalize word posterior probabilities for all target words occurring in the current sentence
 */ 
void NBestWordPostLev::normalizePosterior() {
   for (vector<ConfScore>::iterator itr=conf.begin(); itr!=conf.end(); itr++)
      itr->normalize(totalProb);
}

/**
 * Determine the value for the whole sentence from word posterior probabilities
 * for given sentence 'trg'
 * -> values for whole sentence which can be used for rescoring
 */
double NBestWordPostLev::sentPosteriorOne() {

   if (getNBsize()==0)
      return INFINITY;
   else {
      assert(conf.size()==trg.size());  
      return sentPosteriorScores(conf);
   }
}

/**
 * Determine and return all word posterior probabilities
 * for given sentence 'trg'
 */
vector<double> NBestWordPostLev::wordPosteriorsOne() {

   if (getNBsize()==0)
      return vector<double>(1,INFINITY);
   else {
      assert(conf.size()==trg.size());  
      return wordPosteriorProbs(conf);
   }
}

/**
 * Compute and output the word posterior probabilities for all words in the given hypothesis 'trg'
 */
void NBestWordPostLev::tagPosteriorOne(ostream &out, const int &format) {

   assert(conf.size()==trg.size());  
   printPosteriorScores(out,conf,trg,format);
}


/**
 * Compute and output the word posterior probabilities for all words of all hypotheses in 'nbest'
 */
void NBestWordPostLev::tagPosteriorAll(ostream &out, const int &format) {

   for (uint n=0; n<min(N,Ntag); n++) {
      trg = nbest[n].getTokens();
      tagPosteriorOne(out,format);
   }
}

/**
 * Determine the value for the whole sentence from word posterior probabilities
 * for each sentence in nbest
 * -> values for whole sentence which can be used for rescoring
 */
void NBestWordPostLev::tagSentPosteriorAll(ostream &out) {

   for (uint n=0; n<min(N,Ntag); n++) {    
      trg = nbest[n].getTokens();
      printSentPosteriorScores(out,conf);
   } // for n
}
