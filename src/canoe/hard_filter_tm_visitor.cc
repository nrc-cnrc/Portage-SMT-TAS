/**
 * @author Samuel Larkin
 * @file hard_filter_tm_visitor.cc  hard-limit phrase table filter based on
 *                                  filter_joint
 *
 * $Id$
 *
 * 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#include "hard_filter_tm_visitor.h"

using namespace Portage;
using namespace Portage::Joint_Filtering;

/// Phrase Table filter joint logger
//Logging::logger ptLogger_hardFilterTMVisitor(Logging::getLogger("debug.canoe.hardFilterTMVisitor"));


hardFilterTMVisitor::PhraseInfo4HardFiltering::PhraseInfo4HardFiltering(const pair<const Phrase, TScore>* ref, 
   const Uint numTextTransModels, 
   double log_almost_0, 
   const vector<double>& hard_filter_weights)
: Parent(ref)
{
   assert(numTextTransModels>0);
   const TScore& tScores(ref->second);
   phrase = ref->first;

   MakeLogProbs(forward_trans_probs, tScores.forward, numTextTransModels, log_almost_0);
   MakeLogProbs(phrase_trans_probs, tScores.backward, numTextTransModels, log_almost_0);

   forward_trans_prob  = dotProduct(forward_trans_probs, hard_filter_weights, numTextTransModels);
   phrase_trans_prob   = dotProduct(phrase_trans_probs,  hard_filter_weights, numTextTransModels);
}



hardFilterTMVisitor::hardFilterTMVisitor(
        const PhraseTable &parent,
        double log_almost_0,
        const vector<double>* const hard_filter_weights)
: Parent(parent, log_almost_0, "HARD")
, hard_filter_weights(hard_filter_weights)
{
   //LOG_VERBOSE4(ptLogger_hardFilterTMVisitor, "hard filter_joint LOG probs with resized converted");
   cerr << join(*hard_filter_weights) << endl;
}


void hardFilterTMVisitor::operator()(TargetPhraseTable& tgtTable)
{
   assert(hard_filter_weights);

   // only applicable if there is more than L entries in the table
   if (tgtTable.size() > L) {
      if (!hard_filter_weights->empty() && hard_filter_weights->size() != numTextTransModels)
         error(ETFatal, "Invalid number of TM weights for filter_joint hard (%d, %d)", hard_filter_weights->size(), numTextTransModels);

      // Store all phrases into a vector, along with their weight
      typedef vector<pair<double, PhraseInfo4filtering*> > FilteringInfo;
      FilteringInfo phrases;
      phrases.reserve(tgtTable.size());

      // for each entry in tgtTable create an object with enough info to sort
      // them all in a way that the dynamic algo will work correctly
      for (TargetPhraseTable::const_iterator it = tgtTable.begin(); it != tgtTable.end(); ++it) {
         PhraseInfo4HardFiltering* newPI = new PhraseInfo4HardFiltering(&(*it), numTextTransModels, log_almost_0, *hard_filter_weights);
         phrases.push_back(make_pair(newPI->forward_trans_prob, newPI));
      }

      // Keep the first L which we are sure to keep
      TargetPhraseTable reduceTgtTable;
      //reduceTgtTable.reserve(L);

      // Use a heap to extract the pruneSize best items
      make_heap(phrases.begin(), phrases.end(), phraseLessThan);
      for (Uint i(0); i<L; ++i) {
         pop_heap(phrases.begin(), phrases.end(), phraseLessThan);
         pair<double, PhraseInfo4filtering*>& element = phrases.back();
         assert(element.second);
         reduceTgtTable.insert(*(element.second->ref));
         delete element.second;
         phrases.pop_back();
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

