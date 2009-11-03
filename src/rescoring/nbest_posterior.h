/**
 * @author Nicola Ueffing
 * @file nbest_posterior.h Base class for calculating word/phrase posterior
 * probabilities over N-best lists; different variants are implemented in
 * nbest_wordpost_lev.h, nbest_wordpost_src.h, ...
 *
 * $Id$
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
*/

#ifndef NBEST_POSTERIOR_H
#define NBEST_POSTERIOR_H

#include "confidence_score.h"
#include "featurefunction_set.h"
#include <map>
#include <iostream>
#include <cmath>

namespace Portage
{

   const Uint MAXSIZE=100000;

   struct trgPhraseLessThan {
      bool operator()(const Tokens &p1, const Tokens &p2) {
         if (p1.size() < p2.size()) return true;
         if (p1.size() > p2.size()) return false;
         for (Uint i=0; i<p1.size(); i++) {
            if (p1[i]<p2[i]) return true;
            if (p1[i]>p2[i]) return false;
         }
         return false;
      }
   };


   /**
    * Elements of NBestPosterior:
    * - nbest:  N-best list
    * - scores: sentence scores for target sentences in N-best list
    * - weights:weights of the different submodels (features)
    * - N:      actual length of N-best list
    * - Ntag:   number of sentences in the N-best list which are to be tagged with posterior probabilities
    * - Nbasis: number of sentences in the N-best list on which the calculation is based
    *   (can be smaller than actual length N to speed up calculation,
    *   but cannot be smaller than Ntag of course)
    * - totalProb: total probability mass, rank sum, length of the N-best list (needed for normalization)
    * - scale: scaling factor for probability of each sentence
    * - trg: target sentence which is to be tagged with posterior probabilities
    */
   class NBestPosterior {
      protected:
         Nbest               nbest;
         uVector             scores, weights;
         FeatureFunctionSet  ffset;
         Uint                N, Ntag, Nbasis;
         ConfScore           totalProb;
         double              scale;
         Tokens              trg;

      public:
         NBestPosterior() : nbest(), Ntag(MAXSIZE), Nbasis(MAXSIZE), totalProb(), scale(1), trg(Tokens()) {}
         NBestPosterior(const Tokens &t) : nbest(), Ntag(MAXSIZE), Nbasis(MAXSIZE), totalProb(), scale(1), trg(t) {}
         virtual ~NBestPosterior();

         void init();
         virtual void init(const Tokens &t);
         void init(const Sentence &t);
         void setFFProperties(const string& ffval_wts_file, const string& prefix, double scale);
         void clear();
         void clearAll();
         void setNB(const Nbest &nb);
         int  getNBsize() {return nbest.size(); }
         void setScale(double s);
         void setFFSet(const string &args);

         /**
          * Sets a new alignment
          * @param al  alignment
          */
         virtual void   setAlig(Alignment &al) {}

         /**
          * Sets a new maximum N for Ngram
          * @param m   maximum N
          */
         virtual void   setMaxN(Uint m) {}

         /// @param src_sent_id  source sentence index
         virtual void   computePosterior(Uint src_sent_id) =0;
         virtual void   normalizePosterior() =0;
         virtual double sentPosteriorOne() =0;
         virtual vector<double> wordPosteriorsOne() =0;
         virtual void   tagPosteriorOne(ostream &out, int format) =0;
         virtual void   tagPosteriorAll(ostream &out, int format) =0;
         virtual void   tagSentPosteriorAll(ostream &out) =0;

         void   printPosteriorScores(ostream &out, const vector<ConfScore> &v, const Tokens &trg, int format=0);

         /**
          * Output the min, max, sum, product, arithmetic mean and geometric
          * mean over confidence values for all words in a sentence
          */
         void   printSentPosteriorScores(ostream &out, const vector<ConfScore> &v);

         /**
          * Return the product over confidence values for all words in a
          * sentence
          */
         double sentPosteriorScores(const vector<ConfScore> &v);

         /**
          * Return all word posterior probabilities for a sentence
          */
         vector<double> wordPosteriorProbs(const vector<ConfScore> &v);
   };

}

#endif
