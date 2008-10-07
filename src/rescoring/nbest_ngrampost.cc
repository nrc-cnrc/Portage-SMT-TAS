/**
 * @author Nicola Ueffing
 * @file nbest_ngrampost.cc  Implementation of NBestNgramPost.
 *
 *
 * COMMENTS: derived class for calculating ngram posterior probabilities over N-best lists
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "nbest_ngrampost.h"

using namespace Portage;

/**
 * Compute n-gram posterior probabilities for all target n-grams occurring in the N-best list
 */
void NBestNgramPost::computePosterior(Uint src_sent_id) {

   Ngram2Posterior.clear();
   Ngram2Posterior.init();
   NBestPosterior::computePosterior(src_sent_id);

   vector<string> ng;
   for (uint n=0; n<min(Nbasis,nbest.size()); n++) {

      const Tokens& hypn = nbest[n].getTokens();
      if (!hypn.empty()) {

         /*
          * Note: totalProb is not needed here because the tree stores the total prob. mass in the root
          */
         // totalProb.update(scores[n], n);

         ng.clear();
         ng.reserve(hypn.size());
         for (Uint i=0; i<min(maxN, hypn.size()); i++) {
            /*
             * construct n-grams and update in tree
             */
            ng.push_back(hypn[i]);
            Ngram2Posterior.update(ng,scores[n],n);

            /*
               cerr << ng.size() << "-gram from   0 to " << i << " '";
               for (vector<string>::const_iterator itr=ng.begin(); itr!=ng.end(); itr++)
               cerr << *itr << " ";
               cerr << "' " << scores[n] << ", " << n << " -> " << Ngram2Posterior.getPost(ng).prob() << ", " << Ngram2Posterior.getPost(ng).rank() << endl;
             */

         } // for i
         for (uint i=maxN; i<hypn.size(); i++) {
            /*
             * construct n-grams and update in tree
             */
            ng.erase(ng.begin());
            ng.push_back(hypn[i]);
            Ngram2Posterior.update(ng,scores[n],n);

            /*
               cerr << ng.size() << "-gram from " << i-maxN+1 << " to " << i << " '";
               for (vector<string>::const_iterator itr=ng.begin(); itr!=ng.end(); itr++)
               cerr << *itr << " ";
               cerr << "' " << scores[n] << ", " << n << " -> " << Ngram2Posterior.getPost(ng).prob() << ", " << Ngram2Posterior.getPost(ng).rank() << endl;
             */

         } // for i
         /*
          * last hyp. words with </s> ???

          for (uint i=hypn.size()-maxN+1; i<hypn.size(); i++) {
         // construct n-grams and update in tree
         ng.erase(ng.begin());
         Ngram2Posterior.update(ng,scores[n],n);

         cerr << ng.size() << "-gram from " << i << " to " << hypn.size()-1 << " '";
         for (vector<string>::const_iterator itr=ng.begin(); itr!=ng.end(); itr++)
         cerr << *itr << " ";
         cerr << "' " << scores[n] << " " << n << endl;
         } // for i
          */
      } // if hyp. n is non-empty
   } // for  n

   normalizePosterior();
}


/**
 * Normalize word posterior probabilities for all target words occurring in the N-best list
 */
void NBestNgramPost::normalizePosterior() {
   /*
    * Note: totalProb is not needed here because the tree stores the total prob. mass in the root
    */
   Ngram2Posterior.normalize();
}

/**
 * Determine the value for the whole sentence from word confidences
 * for given sentence 'trg'
 * -> values for whole sentence which can be used for rescoring
 */
double NBestNgramPost::sentPosteriorOne() {

   assert(Ngram2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(trg.size());

   vector<string> ngram;
   ngram.reserve(trg.size());

   for (uint i=0; i<min(maxN, trg.size()); i++) {
      ngram.push_back(trg[i]);
      conf.push_back(Ngram2Posterior.getPost(ngram));
      // cerr << "post. costs of " << ngram.size() << "-gram starting at 0 are " << conf.back().prob() << endl;
   }

   for (uint i=maxN; i<trg.size(); i++) {
      ngram.push_back(trg[i]);
      ngram.erase(ngram.begin());
      conf.push_back(Ngram2Posterior.getPost(ngram));
      // cerr << "post. costs of " << ngram.size() << "-gram starting at " << i << " are " << conf.back().prob() << endl;
   } // for i
   return sentPosteriorScores(conf);
}

/**
 * Determine and return all word posterior probabilities
 * for given sentence 'trg'
 */
vector<double> NBestNgramPost::wordPosteriorsOne() {

   assert(Ngram2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(trg.size());

   vector<string> ngram;
   ngram.reserve(trg.size());

   for (uint i=0; i<min(maxN, trg.size()); i++) {
      ngram.push_back(trg[i]);
      conf.push_back(Ngram2Posterior.getPost(ngram));
   }

   for (uint i=maxN; i<trg.size(); i++) {
      ngram.push_back(trg[i]);
      ngram.erase(ngram.begin());
      conf.push_back(Ngram2Posterior.getPost(ngram));
   } // for i
   assert(conf.size() == trg.size());
   return wordPosteriorProbs(conf);
}

/**
 * Output the word posterior probabilities for all words in the given hypothesis 'trg'
 */
void NBestNgramPost::tagPosteriorOne(ostream &out, int format) {

   assert(Ngram2Posterior.size());

   vector<ConfScore> conf;
   conf.reserve(trg.size());

   //for (uint i=0; i<trg.size(); i++)
   // conf.push_back(Ngram2Posterior[pair<Token,int>(trg[i],i)]);

   printPosteriorScores(out,conf,trg,format);
}


/**
 * Output the word posterior probabilities for all words of all hypotheses in 'nbest'
 */
void NBestNgramPost::tagPosteriorAll(ostream &out, int format) {

   assert(Ngram2Posterior.size());

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
void NBestNgramPost::tagSentPosteriorAll(ostream &out) {

   assert(Ngram2Posterior.size());

   vector<ConfScore> conf;
   for (uint n=0; n<min(N,Ntag); n++) {

      trg = nbest[n].getTokens();
      conf.clear();
      conf.reserve(trg.size());

      for (uint i=0; i<trg.size(); i++) {
         /*
         pair<Token,int> p(trg[i],i);
         assert(Ngram2Posterior.find(p) != Ngram2Posterior.end());
         conf.push_back(Ngram2Posterior[p]);
          */
      } // for i
      printSentPosteriorScores(out,conf);
   } // for n
}
