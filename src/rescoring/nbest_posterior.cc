/**
 * @author Nicola Ueffing
 * @file nbest_posterior.cc Implementation of base class for calculating
 * posterior probabilities over N-best lists; different variants are implemented
 * in nbest_confidence_lev.cc, nbest_confidence_src.cc, ...
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "nbest_posterior.h"

using namespace Portage;

NBestPosterior::~NBestPosterior() {};

void NBestPosterior::init() {
   /***/
}
void NBestPosterior::init(const Tokens &t) {
   trg = t;
}
void NBestPosterior::init(const Sentence &t) {
   init(t.getTokens());
}

void NBestPosterior::setFFProperties(const string& ffval_wts_file, const string& prefix, const double scale) {
   assert(!ffval_wts_file.empty());
   ffset.read(ffval_wts_file, true, prefix.c_str(), false); 
   ffset.getWeights(weights);
   weights *= scale;
}

void NBestPosterior::clearAll() {
   totalProb.reset();
   trg.clear();
}
void NBestPosterior::setNB(const Nbest &nb) {
   nbest = nb;
   N = nbest.size();
}

void NBestPosterior::computePosterior(const Uint src_sent_id) {

   // Guarding against multiple calls to computePosterior for a particular
   // source sentence.
   static int sourceSentenceGuard = -1;
   if (sourceSentenceGuard == (int)src_sent_id) return;
   sourceSentenceGuard = src_sent_id;

   uMatrix scoreMatrix;
   scoreMatrix.clear();
   ffset.computeFFMatrix(scoreMatrix, src_sent_id, nbest);
   if (nbest.size()>0)
      scores = boost::numeric::ublas::prec_prod(scoreMatrix, weights);
   else
     scores.resize(0);
   // SAM what if the nbest list is empty => what is the content of scores??
   totalProb.reset();
}

void NBestPosterior::printPosteriorScores(ostream &out, const vector<ConfScore> &v, const Tokens &trg, const int format) {
   if (format==0)
      for (vector<ConfScore>::const_iterator itr=v.begin(); itr!=v.end(); itr++)
         out << itr->prob() << " " << itr->rank() << " " << itr->relfreq() << " / ";
   else if (format==1)
      for (uint i=0; i<trg.size(); i++) 
         out << trg[i] << "_{" << v[i].prob() << "}_{" << v[i].rank() << "}_{" << v[i].relfreq() << "} ";  
   out << endl; 
   out.flush();
}

void NBestPosterior::printSentPosteriorScores(ostream &out, const vector<ConfScore> &v) {
   double logprod=0, minp=1000, maxp=0, geop, c=0;
   for (vector<ConfScore>::const_iterator itr=v.begin(); itr!=v.end(); itr++) {
      c = itr->prob();
      logprod += c;
      minp    = min(c,minp);
      maxp    = max(c,maxp);
   } // for itr
   geop  = logprod/double(v.size());

   out << logprod << " " << minp << " " <<  maxp << " " << geop << endl;
   out.flush();
} 

double NBestPosterior::sentPosteriorScores(const vector<ConfScore> &v) {
   double logprod=0;
   for (vector<ConfScore>::const_iterator itr=v.begin(); itr!=v.end(); itr++) 
      logprod += itr->prob();
   return logprod;
} 

vector<double> NBestPosterior::wordPosteriorProbs(const vector<ConfScore> &v) {
   vector<double> result;
   for (vector<ConfScore>::const_iterator itr=v.begin(); itr!=v.end(); itr++) 
      result.push_back(itr->prob());
   return result;
} 
