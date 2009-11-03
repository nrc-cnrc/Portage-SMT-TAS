/**
 * $Id$
 * @file ngrammatch_feature.cc
 *   implementation of feature which is only used for (semi-)forced alignment of a 
 *   source-target sentence pair: 
 *   compute unigram match between a hypothesis and the actual target sentence
 *
 * PROGRAMMER: Nicola Ueffing
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "ngrammatch_feature.h"
#include <iostream>

using namespace Portage;

NgramMatchFeature::NgramMatchFeature(const string& args) :
  N(atoi(args.c_str()))
{}



void NgramMatchFeature::newSrcSent(const newSrcSentInfo& info) {
  // This indicates that the user gave a target sentence.
  assert(info.tgt_sent != NULL);
  refNgrams.clear();
  // construct all possible ngrams from tgt words
  for (Uint i=0; i + N <= info.tgt_sent_ids.size(); ++i) { 
    NGram<Uint> ngri(info.tgt_sent_ids, i, N);
    refNgrams[ngri]++;
  } 
  refLen = info.tgt_sent_ids.size();
}


double NgramMatchFeature::precomputeFutureScore(const PhraseInfo& phrase_info) { 
  return double(-maxNgramMatch(phrase_info.phrase));
}


double NgramMatchFeature::futureScore(const PartialTranslation &trans)  { 
  return 0;
}


double NgramMatchFeature::score(const PartialTranslation& pt) {
   // optimized like levenshtein by storing some values to be used again     

  assert(pt.levInfo != NULL);
  double dist(0.0f);
  vector<double>& ngramDistance = const_cast<vector<double>&>(pt.levInfo->ngramDistance);
  
  if ((int)ngramDistance.size() < N)  // higher ngram, which is not calculated before
     ngramDistance.resize(N, -1);

  if (ngramDistance[N-1] == -1){    // not calculated before
     dist = ngramMatch(pt);
     // store the n-gram match distance for future use
     ngramDistance[N-1] = dist;
  } 
  else
     dist = ngramDistance[N-1];                      // has been calculated before

  if (pt.back->getLength() > 0)
     dist -= pt.back->levInfo->ngramDistance[N-1];

  if (dist<0) {
     cerr << "Neg. dist: " << dist << " = " << ngramDistance[N-1] << " - " << pt.back->levInfo->ngramDistance[N-1] << endl;
     std::cerr << "refNgrams:";
     for (map<NGram<Uint>,int>::const_iterator itr=refNgrams.begin(); itr!=refNgrams.end(); itr++)
        cerr << " " << itr->first << "," << itr->second << " ";
     Phrase hyp;
     pt.getEntirePhrase(hyp);
     cerr << endl << "new ";
     if (pt.sourceWordsNotCovered.empty())
        cerr << "complete ";
     cerr << "hyp:";
     for (Phrase::const_iterator ii=hyp.begin(); ii!=hyp.end(); ii++)
        cerr << " " << *ii;
     hyp.clear();
     pt.back->getEntirePhrase(hyp);
     cerr << endl << "old hyp:";
     for (Phrase::const_iterator ii=hyp.begin(); ii!=hyp.end(); ii++)
        cerr << " " << *ii;
     cerr << endl;
  }

  assert(dist>=0);
  return -dist;
}

int NgramMatchFeature::ngramMatch(const PartialTranslation& pt) {
  Phrase hyp;
  pt.getEntirePhrase(hyp);
  
  /**
   * For complete translations, consider also deletions of ref. words
   * For partial translations, determine maximal ngram match between
   *     hyp. and ref.
   */
  
  int dist = maxNgramMatch(hyp);
  
  if (pt.sourceWordsNotCovered.empty()) {
    if (refLen > hyp.size())
      dist += (refLen - hyp.size());
  }
  return dist;
}


int NgramMatchFeature::maxNgramMatch(const Phrase& phr) {

  if ((int)phr.size()<N)
    return 0;

  int dist=0;
  map<NGram<Uint>, int> phrNgrams;

  for (Uint i=0; i + N <= phr.size(); ++i) {
    NGram<Uint> ngri(phr, i, N);
    // count only frequencies of words which occur in the reference as well
    if (refNgrams.find(ngri) == refNgrams.end())
      dist++;
    else {
      phrNgrams[ngri]++;
      if (phrNgrams[ngri] > refNgrams[ngri])
        dist++;
    }
  } // for i 
  return dist;
}


Uint NgramMatchFeature::computeRecombHash(const PartialTranslation &pt) {
  return 0;
}

bool NgramMatchFeature::isRecombinable(const PartialTranslation &pt1,
                                       const PartialTranslation &pt2) {

  // Check whether the length of the translations is the same
  if (pt1.getLength() != pt2.getLength())
    return false;

  // Check whether the n-gram distance accumulated so far is the same
  assert(pt1.levInfo != NULL);
  assert(pt2.levInfo != NULL);
  assert((int)pt1.levInfo->ngramDistance.size() > N-1);
  assert((int)pt2.levInfo->ngramDistance.size() > N-1);
  const double dist1 = pt1.levInfo->ngramDistance[N-1]; 
  const double dist2 = pt2.levInfo->ngramDistance[N-1]; 
  if (dist1 != dist2)
    return false;

  // Check whether the last N-1 words are the same
  if (N>1)
    if (! pt1.sameLastWords(pt2, N-1, 0))
      return false;

  // Check whether the matching n-grams are the same
  // TO BE IMPLEMENTED ???

  return true;
}

