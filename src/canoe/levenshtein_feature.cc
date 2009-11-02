/**
 * $Id$
 * @file levenshtein_feature.cc  
 *   implementation of feature which is only used for (semi-)forced alignment
 *   of a source-target sentence pair: 
 *   compute Levenshtein distance between a hypothesis and the actual target
 *   sentence
 *
 * PROGRAMMER: Nicola Ueffing
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada / 
 * Copyright 2006, Her Majesty in Right of Canada
 */

#include "levenshtein_feature.h"
#include "basicmodel.h"
#include <set>
#include <math.h>

namespace Portage {
        
   LevenshteinFeature::LevenshteinFeature(BasicModelGenerator* bmg) 
   : levLimit(bmg->c->levLimit)
   , verbosity(bmg->c->verbosity)
   {
   }

  void LevenshteinFeature::newSrcSent(const newSrcSentInfo& nss_info) {
     ref = nss_info.tgt_sent_ids;

     if (verbosity>1) cerr << "First Limit = " << levLimit << " ";
     if (levLimit != NO_MAX_LEVENSHTEIN)
        // change to relative levLimit with respect to the source sent length
        relLevLimit = (int)((double)(nss_info.src_sent.size()*levLimit)/(double)100);
     else relLevLimit = NO_MAX_LEVENSHTEIN;

     if (verbosity>1) cerr << "Sent_size = " << nss_info.src_sent.size() << " ";     
     if (verbosity>1) cerr << "Limit = " << relLevLimit << " ";    
  }


  double LevenshteinFeature::precomputeFutureScore(const PhraseInfo& phrase_info) { 

     const double minDist = minLevDist(phrase_info.phrase);

     assert(minDist>=0);

     // cut off those out of lev limit
     if (levLimit != NO_MAX_LEVENSHTEIN && minDist > relLevLimit) return -INFINITY;

     return -minDist;
  }

  
  double LevenshteinFeature::futureScore(const PartialTranslation &trans)  { 
     return 0;
  }

  
  double LevenshteinFeature::score(const PartialTranslation& pt) {

     assert(pt.levInfo != NULL);
     // optimized by storing at least the levscore in the prev translation    
     int dist(0);

     if (pt.levInfo->levDistance == -1){  // not calculated before
        dist = levDist(pt);  
        // store the levenshtein distance for future use
        const_cast<PartialTranslation&>(pt).levInfo->levDistance = dist;
     } 
     else
        dist = pt.levInfo->levDistance;                      // has been calculated before


     // cut off those out of lev limit
     if (levLimit != NO_MAX_LEVENSHTEIN && dist > relLevLimit) return -INFINITY;

     if (pt.back->getLength()>0)  
        dist -= pt.back->levInfo->levDistance;

     assert(dist>=0);

     return -dist;
  }


  int LevenshteinFeature::levDist(const PartialTranslation& pt) {
     Phrase hyp;
     pt.getEntirePhrase(hyp);

     /*
      * For complete translations, consider also deletions of ref. words
      * For partial translations, determine Levenshtein distance between
      *     hyp. and best matching part of ref.
      */
     if (pt.sourceWordsNotCovered.empty()) {
        return lev.LevenDist(hyp, ref);
     }
     else {
        assert(pt.levInfo);
        return partialLevDist(hyp, &(const_cast<PartialTranslation&>(pt).levInfo->min_positions));
     }    
  }


  int LevenshteinFeature::minLevDist(const Phrase& phr) {

     assert(ref.size()>0);
     int dist = phr.size(); // distance if no matching part is found

     set<Uint> phrW;
     for (Phrase::const_iterator itr=phr.begin(); itr!=phr.end(); ++itr)
        phrW.insert(*itr);

     // The first and the last word of the best matching part of the ref. must
     // be contained in the phrase -> move start and end point accordingly
     vector<Uint>::const_iterator start=ref.begin(), end=ref.end()-1;
     while(start != ref.end() && phrW.find(*start) == phrW.end())
        start++;
     while(end+1 != start && phrW.find(*end) == phrW.end())
        end--;
     int refchunklen = end-start;

     // None of the ref. words is contained in the phrase
     if (refchunklen < 0) {
        return dist;
     }
     // The phrase has only 1 word which is found in the reference
     if (dist==1) {
        return 0;
     }
     // Only 1 or only 2 neighbouring ref. words are found in the phrase,
     // i.e., all but these 1 or 2 words in the phrase have been inserted
     // which yields a distance of dist-1 / dist-2
     switch (refchunklen) {
        case 0:
           return dist-1;
        case 1:
           return dist-2;
     }

     int incompleteRef = 2;  // coverage of the ref. from anywhere to anywhere
     vector<Uint> partialRef;
     partialRef.assign(start, end);
     dist = min(dist, lev.LevenDistIncompleteRef(phr, partialRef, incompleteRef));

     return dist;
  }
  
  int LevenshteinFeature::partialLevDist(const Phrase& hyp, boost::dynamic_bitset<>* min_positions) {

     assert(ref.size()>0);

     set<Uint> phrW;
     for (Phrase::const_iterator itr=hyp.begin(); itr!=hyp.end(); itr++)
        phrW.insert(*itr);

     /** The last word of the best matching part of the ref. must
      * be contained in the partial trans -> move end point accordingly
      */
     vector<Uint>::const_iterator start=ref.begin(), end=ref.end()-1;
     while(phrW.find(*end) == phrW.end() && end != ref.begin())
        end--;

     int incompleteRef = 1;  // coverage of the ref. from the beginning
     vector<Uint> partialRef;
     partialRef.assign(start, end+1);

     return lev.LevenDistIncompleteRef(hyp, partialRef, incompleteRef, min_positions);
  }

  Uint LevenshteinFeature::computeRecombHash(const PartialTranslation &pt) {
      return 0;
  }

  bool LevenshteinFeature::isRecombinable(const PartialTranslation &pt1,
                                          const PartialTranslation &pt2) {

     assert(ref.size()>0);
     assert(pt1.levInfo != NULL);
     assert(pt2.levInfo != NULL);

     const int dist1 = pt1.levInfo->levDistance;
     const int dist2 = pt2.levInfo->levDistance;
     assert(dist1 != -1);
     assert(dist2 != -1);

     return (dist1 == dist2)
         && (pt1.levInfo->min_positions == pt2.levInfo->min_positions);
  }

} // Portage

