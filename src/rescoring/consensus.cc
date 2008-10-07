/**
 * @author George Foster, Nicola Ueffing
 * @file consensus.cc  Implementation of ConsensusWer and ConsensusWin.
 * 
 * 
 * COMMENTS: 
 *
 * Consensus over nbest lists:
 *   ConsensusWer: original variant based on WER (determining the Levenshtein alignment 
 *                 which is computationally expensive)
 *   ConsensusWin: modified variant which performs windowing over the neighbouring positions,
 *                 approximation which speeds up computation
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include <wer.h>
#include "consensus.h"

using namespace Portage;

//////////////////////////////////////////////////////////////////////////////
// CONSENSUS WER
ConsensusWer::ConsensusWer(const string& dontcare)
: FeatureFunction(dontcare)
{}

bool ConsensusWer::parseAndCheckArgs()
{
   if (!argument.empty())
      error(ETWarn, "ConsensusWer doesn't require arguments");
   return true;
}

void ConsensusWer::source(Uint s, const Nbest * const nbest)
{
   FeatureFunction::source(s, nbest);

   scores.clear();
   scores.resize(K);

   int emptyhyps = 0;

   for (Uint i = 0; i < K; ++i) {

     if ((*nbest)[i].getTokens().empty()) {
       scores[i] = 0;
       emptyhyps++;
       //cerr << "empty hyp " << i << endl;
     }
     else {
       
       for (Uint j = i+1; j < K; ++j) {
         
         if ((*nbest)[j].getTokens().size() != 0) {
           
           double w = find_mWER((*nbest)[i].getTokens(), (*nbest)[j].getTokens());
           
           //cerr << "distance " << w << " between hyps. " << i << " and " << j << endl;
           scores[i] += w;
           scores[j] += w;
         }
         
       } // for j
     } // if hyp. i is non-empty
   } // for i
   /**
    * Actual length of N-best list
    */
   K -= emptyhyps;
   for (vector<double>::iterator ii=scores.begin(); ii!=scores.end(); ii++)
     (*ii) /= K;
}


//////////////////////////////////////////////////////////////////////////////
// CONSENSUS WINDOW
ConsensusWin::ConsensusWin(const string& dontcare)
: FeatureFunction(dontcare)
{}

bool ConsensusWin::parseAndCheckArgs()
{
   if (!argument.empty())
      error(ETWarn, "ConsensusWin doesn't require arguments");
   return true;
}

void ConsensusWin::source(Uint s, const Nbest * const nbest)
{
   FeatureFunction::source(s, nbest);

   scores.clear();
   scores.resize(K);

   int emptyhyps = 0;

   for (Uint i = 0; i < K; ++i) {

     Tokens hypi = (*nbest)[i].getTokens();
     Uint   leni = hypi.size();

     if (leni==0) {
       scores[i] = 0;
       emptyhyps++;
     }
     else {
       
       for (Uint j = i+1; j < K; ++j) {
         
         if ((*nbest)[j].getTokens().size()>0) {
           
           double w = 0.0;
           
           Tokens hyp1, hyp2;
           Uint   len1, len2;
           // determine shorter hyp. -> hyp1
           if (leni < (*nbest)[j].getTokens().size()) {
             hyp1 = hypi;
             hyp2 = (*nbest)[j].getTokens();
             len1 = leni;
             len2 = hyp2.size();
           }
           else {
             hyp1 = (*nbest)[j].getTokens();
             hyp2 = hypi;
             len1 = hyp1.size();
             len2 = leni;
           }
           
           /** 
            * If hyp1 has only 1 word
            */
           if (len1==1) {
             if (len2==1)
               w = (hyp1[0]==hyp2[0]) ? 0.0 : 2.0;
             else {
               w = (hyp1[0]==hyp2[0] || hyp1[0]==hyp2[1]) ? 0.0 : 2.0;
               w += double(len2 - len1);
             }
           }
           else {
             /**
              * For each word in hyp2, store whether it can be aligned to a word in hyp1
              */
             vector<bool> aligned(len2,false);
             /**
              * Compare first word of hyp1 to the first 2 words of hyp2
              */
             if (hyp1[0]==hyp2[0])
               aligned[0] = true;
             else if (hyp1[0]==hyp2[1])
               aligned[1] = true;
             else
               w = 1.0;
             for (uint k=1; k<len1-1; k++) {
               bool found = false;
               for (uint l = k-1; l<k+2; l++)
                 if (!aligned[l])
                   if (hyp1[k] == hyp2[l]) {
                     aligned[l] = true;
                     found      = true;
                   }
               w += (found) ? 0.0 : 1.0;
             } // for k
             /**
              * Compare last word of hyp1 to 2 or 3 words of hyp2 (depending on len2)
              */
             bool found = false;
             for (uint l = len1-1; l<min(len1+1,len2); l++)
               if (!aligned[l])
                 if (hyp1.back() == hyp2[l]) {
                   aligned[l] = true;
                   found      = true;
                 }
             w += (found) ? 0.0 : 1.0;
             /**
              * Add uncovered words in hyp2
              */
             for (vector<bool>::const_iterator ii=aligned.begin(); ii!=aligned.end(); ii++)
               w += 1.0-int(*ii);
           } // for k
           
           w /= 2.0;
           // cerr << "distance " << w << " between hyps. " << i << " and " << j << endl;
           
           scores[i] += w;
           scores[j] += w;
         } // if hyp. j is non-empty       
       } // for j
     } // if hyp. i is non-empty
   } // for i
   
   /**
    * Actual length of N-best list
    */
   K -= emptyhyps;
   for (vector<double>::iterator ii=scores.begin(); ii!=scores.end(); ii++)
     (*ii) /= K;
}
