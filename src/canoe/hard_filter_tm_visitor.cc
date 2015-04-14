/**
 * @author Samuel Larkin
 * @file hard_filter_tm_visitor.cc  hard-limit phrase table filter based on
 *                                  filter_joint
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



hardFilterTMVisitor::PhraseInfo4HardFiltering::PhraseInfo4HardFiltering(const pair<const Phrase, TScore>* ref, 
   const Uint numTextTransModels, 
   double log_almost_0, 
   const vector<double>& hard_filter_forward_weights,
   const vector<double>& hard_filter_backward_weights,
   bool combined)
: Parent(ref)
{
   assert(numTextTransModels>0);
   const TScore& tScores(ref->second);
   phrase = ref->first;

   MakeLogProbs(forward_trans_probs, tScores.forward, numTextTransModels, log_almost_0);
   MakeLogProbs(phrase_trans_probs, tScores.backward, numTextTransModels, log_almost_0);

   forward_trans_prob  = dotProduct(hard_filter_forward_weights, forward_trans_probs, numTextTransModels);
   phrase_trans_prob   = dotProduct(hard_filter_backward_weights, phrase_trans_probs, numTextTransModels);

   if (combined) {
      // In combined and full mode, the main filtering score is the sum of both forward
      // and backward probs, i.e., all the information we have available in filter_models.
      // TODO: For full, we would also like to add the LM heuristic score, but we don't
      // have that in place yet.
      partial_score = forward_trans_prob + phrase_trans_prob;
   } else {
      // in backward-weights and forward-weights, the main filtering score is the
      // log-linear forward score
      partial_score = forward_trans_prob;
   }
}



hardFilterTMVisitor::hardFilterTMVisitor(
        const PhraseTable &parent,
        double log_almost_0,
        const string& pruningTypeStr,
        vector<double> forwardWeights,
        const vector<double>& transWeights)
: Parent(parent, log_almost_0, "HARD")
{
   hard_filter_backward_weights = transWeights;

   if (forwardWeights.empty())
      forwardWeights = transWeights;

   if (pruningTypeStr == "backward-weights") {
      hard_filter_forward_weights = transWeights;
      combined = false;
      cerr << "Filter forward scores on backward weights: " << join(hard_filter_forward_weights) << endl;
   } else if (pruningTypeStr == "forward-weights") {
      hard_filter_forward_weights = forwardWeights;
      combined = false;
      cerr << "Main filter weights (forward): " << join(hard_filter_forward_weights) << endl;
      cerr << "Backward weights: " << join(hard_filter_backward_weights) << endl;
   } else {
      assert(pruningTypeStr == "combined" || pruningTypeStr == "full");
      combined = true;
      hard_filter_forward_weights = forwardWeights;
      cerr << "Combined weights B: " << join(hard_filter_backward_weights) << " + F: " << join(hard_filter_forward_weights) << endl;
   }
}


void hardFilterTMVisitor::operator()(TargetPhraseTable& tgtTable)
{
   // only applicable if there is more than L entries in the table
   if (tgtTable.size() > L) {
      if (hard_filter_backward_weights.size() != numTextTransModels ||
          hard_filter_forward_weights.size() != numTextTransModels)
         error(ETFatal, "Invalid number of TM weights (%d/%d) for filter_joint hard on %d trans models)",
            hard_filter_forward_weights.size(), hard_filter_backward_weights.size(),
            numTextTransModels);

      // Store all phrases into a vector, along with their weight
      typedef vector<pair<double, PhraseInfo4filtering*> > FilteringInfo;
      FilteringInfo phrases;
      phrases.reserve(tgtTable.size());

      // for each entry in tgtTable create an object with enough info to sort
      // them all in a way that the dynamic algo will work correctly
      for (TargetPhraseTable::const_iterator it = tgtTable.begin(); it != tgtTable.end(); ++it) {
         PhraseInfo4HardFiltering* newPI = new PhraseInfo4HardFiltering(&(*it), numTextTransModels,
               log_almost_0, hard_filter_forward_weights, hard_filter_backward_weights, combined);
         phrases.push_back(make_pair(newPI->partial_score, newPI));
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

