/**
 * @author Samuel Larkin
 * @file soft_filter_tm_visitor.cc  soft-limit phrase table filter based on
 *                                  filter_joint
 *
 * Implements the soft filtering of phrase table
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "soft_filter_tm_visitor.h"
#include <functional>  // for not2
#include "logging.h"

using namespace Portage;
using namespace Portage::Joint_Filtering;


/// Phrase Table filter joint logger
Logging::logger ptLogger_softFilterTMVisitor(Logging::getLogger("debug.canoe.softFilterTMVisitor"));


softFilterTMVisitor::PhraseInfo4SoftFiltering::PhraseInfo4SoftFiltering(const pair<const Phrase, TScore>* ref, 
   const Uint numTextTransModels, 
   double log_almost_0)
: Parent(ref)
{
   assert(numTextTransModels>0);
   const TScore& tScores(ref->second);
   phrase = ref->first;

   MakeLogProbs(forward_trans_probs, tScores.forward, numTextTransModels, log_almost_0);
   MakeLogProbs(phrase_trans_probs, tScores.backward, numTextTransModels, log_almost_0);

   forward_trans_prob  = accumulate(forward_trans_probs.begin(), forward_trans_probs.end(), 0.0f);
   phrase_trans_prob   = accumulate(phrase_trans_probs.begin(), phrase_trans_probs.end(), 0.0f);
}




softFilterTMVisitor::softFilterTMVisitor(
      const PhraseTable &parent,
      double log_almost_0,
      const string& pruningTypeStr)
: Parent(parent, log_almost_0, "SOFT")
, combined(pruningTypeStr == "combined" || pruningTypeStr == "full")
{
   LOG_VERBOSE4(ptLogger_softFilterTMVisitor, "soft filter_joint LOG probs with resized converted");
}


void softFilterTMVisitor::operator()(TargetPhraseTable& tgtTable)
{
   // only applicable if there is more than L entries in the table
   if (tgtTable.size() > L) {
      // Store all phrases into a vector, along with their weight
      typedef vector<pair<double, PhraseInfo4filtering*> > FilteringInfo;
      FilteringInfo phrases;
      phrases.reserve(tgtTable.size());

      // for each entry in tgtTable create an object with enough info to sort
      // them all in a way that the dynamic algo will work correctly
      for (TargetPhraseTable::const_iterator it = tgtTable.begin(); it != tgtTable.end(); ++it) {
         PhraseInfo4SoftFiltering* newPI = new PhraseInfo4SoftFiltering(&*it, numTextTransModels, log_almost_0);
         double filter_sorting_score =
            (combined ? (newPI->forward_trans_prob + newPI->phrase_trans_prob)
                      : newPI->forward_trans_prob);
         phrases.push_back(make_pair(filter_sorting_score, newPI));
      }

      // Sorting for the dynamic algo to work
      sort(phrases.begin(), phrases.end(), phraseGreaterThan);
      // DEBUGGING
      if (false) {
         cerr << "Ordered list: " << endl;
         for (Uint i(0); i<phrases.size(); ++i) {
            cerr << i << ": " << parent.getStringPhrase(phrases[i].second->ref->first) << " ";
            phrases[i].second->print(i);
            cerr << endl;
         }
         cerr << endl;
      }

      // Keep the first L which we are sure to keep
      TargetPhraseTable reduceTgtTable;
      //reduceTgtTable.reserve(L);
      for (Uint i(0); i<L; ++i) {
         reduceTgtTable.insert(*(phrases[i].second->ref));
      }

      // The actual dynamic filtering algo
      vector<Uint> counts(phrases.size(), 0);
      for (Uint i(L); i<phrases.size(); ++i) {
         Uint count(0);
         for (int j(i-1); j>=0; --j) { 
            if ( lessdot(*phrases[i].second, *phrases[j].second) ) {
               //cerr << "result is true : " << parent.getStringPhrase(phrases[i].second->ref->first) << endl;
               ++count;
               if (count + counts[j] >= L) {
                  count = L;
               }
            }
            /*else {
               cerr << "result is false" << endl;
            }// */

            // If j's count is greater then L then we know we won't keep this phrase, so
            // stop.
            // If count+j < L, we know we don't have enough phrases left before j to make
            // discarding j possible, so stop too.
            if (count >= L || count + j < L) {
               break;
            }
         }

         if (count < L) {
            reduceTgtTable.insert(*(phrases[i].second->ref));
         }
         /*else {
            // DEBUGGING
            //LOG_VERBOSE4(ptLogger_filter_joint, "Discarding: count = %d, %s\n", count, parent.getStringPhrase(phrases[i].second->ref->first).c_str());
         }// */
         counts[i] = count;
      }

      LOG_INFO(ptLogger_softFilterTMVisitor, "keeping only: %d / %d", reduceTgtTable.size(), tgtTable.size());
      // DEBUGGING
      if (false) {
         cerr << "Ordered list: " << endl;
         for (Uint i(0); i<phrases.size(); ++i) {
            cerr << i << " =>" << counts[i] << ": " << parent.getStringPhrase(phrases[i].second->ref->first) << endl;
         }
         cerr << endl;
      }

      // exchanging leaf for the reduced set
      tgtTable.swap(reduceTgtTable);
      
      // CLEAN UP
      typedef FilteringInfo::iterator FIT;
      for(FIT fit(phrases.begin()); fit!=phrases.end(); ++fit) {
         if (fit->second) delete fit->second;
      }
   }
}


bool softFilterTMVisitor::lessdot(PhraseInfo& in, PhraseInfo& jn) const
{
   assert(in.forward_trans_probs.size() == jn.forward_trans_probs.size());
   assert(in.phrase_trans_probs.size()  == jn.phrase_trans_probs.size());
   // DEBUGGING
   /*cerr << "in: " << parent.getStringPhrase(in.phrase) << endl;
   cerr << "forward: " << join(in.forward_trans_probs) << " ";
   cerr << "backward: " << join(in.phrase_trans_probs) << endl;
   cerr << "jn: " << parent.getStringPhrase(jn.phrase) << endl;
   cerr << "forward: " << join(jn.forward_trans_probs) << " ";
   cerr << "backward: " << join(jn.phrase_trans_probs) << endl;
   // */

   if (combined) {
      // In combined mode, the first comparison is over all forward and backward scores.
      bool bC(true);
      for (Uint i(0); i<in.forward_trans_probs.size(); ++i) {
         if (in.forward_trans_probs[i] > jn.forward_trans_probs[i]) return false;
         bC = bC && (in.forward_trans_probs[i] < jn.forward_trans_probs[i]);
      }
      for (Uint i(0); i<in.phrase_trans_probs.size(); ++i) {
         if (in.phrase_trans_probs[i] > jn.phrase_trans_probs[i]) return false;
         bC = bC && (in.phrase_trans_probs[i] < jn.phrase_trans_probs[i]);
      }
      if (bC == true) return true;

      // Note: even in combined mode, the second comparison is the same as in non-combined
      // mode, which means we're reversing the comparison direction for
      // phrase_trans_probs. This is what the decoder really does, like it or not, so we
      // have to follow suite in filtering. This means soft filtering is likely to keep
      // more phrase pairs than necessary in combined mode; we just have to live with it.
   } else {
      // First comparison criterion is forward score:
      //  - false if any in.forward > jn.forward
      //  - true if all in.forward < jn.forward
      //  - go on to next criterion if all in.forward <= jn.forward but not all <
      bool bX(true);
      for (Uint i(0); i<in.forward_trans_probs.size(); ++i) {
         if (in.forward_trans_probs[i] > jn.forward_trans_probs[i]) return false;
         bX = bX && (in.forward_trans_probs[i] < jn.forward_trans_probs[i]);
      }
      if (bX == true) return true;
   }

   // Second comparison criterion is backward score in reverse order:
   //  - false if any in.backward < jn.backward
   //  - true if all in.backward > jn.backward
   //  - go on to next criterion if all in.backward >= jn.backward but not all >
   bool bY(true);
   for (Uint i(0); i<in.phrase_trans_probs.size(); ++i) {
      if (in.phrase_trans_probs[i] < jn.phrase_trans_probs[i]) return false;
      bY = bY && (in.phrase_trans_probs[i] > jn.phrase_trans_probs[i]);
   }
   if (bY == true) return true;

   // Third criterion is hashed tie-breaking - invoke the same code canoe uses
   // to make sure the hash is done the same way, with phrase_trans_probs and
   // forward_trans_prob set to equality to make sure it goes down to its third
   // criterion.
   in.phrase_trans_prob  = jn.phrase_trans_prob  = 1;
   in.forward_trans_prob = jn.forward_trans_prob = 1;
   return phraseLessThan(make_pair(1,&in), make_pair(1,&jn));
}

