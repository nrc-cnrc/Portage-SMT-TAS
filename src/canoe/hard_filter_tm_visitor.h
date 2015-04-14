/**
 * @author Samuel Larkin
 * @file hard_filter_tm_visitor.h  Defines the hard filter visitor for phrase tables
 *
 * Defines the hard filter visitor for filtering phrase tables based on
 * filter_joint algorithm
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __HARD_FILTER_TM_VISITOR_H__
#define __HARD_FILTER_TM_VISITOR_H__

#include <phrasetable.h>
#include <filter_tm_visitor.h>


namespace Portage {
/// Namespace to hold all relevant code to joint filtering
namespace Joint_Filtering {
/**
 * Callable entity that implements the hard filter_joint per leaf of the trie.
 */
struct hardFilterTMVisitor : public filterTMVisitor
{
   /**
   * Yet another PhraseInfo which will be used for hard filter_joint.
   */
   struct PhraseInfo4HardFiltering : public filterTMVisitor::PhraseInfo4filtering
   {
      /// Definition of the inherited Parent
      typedef filterTMVisitor::PhraseInfo4filtering Parent;

      /**
      * Default constructor.
      * Keeps a pointer to the original object since after filter_joint, we
      * will need the original to rewrite a reduced target table.
      * @param ref  pointer to the original object used to construct this
      * @param numTextTransModels  number of text translation models.  We need
      * this information since not all entries have the same size, we need to
      * make them the same size=> the same number of probs for each (forward
      * and/or backward).
      * @param log_almost_0  user defined 0
      * @param hard_filter_forward_weights  phrasetable weights for filtering forward scores
      * @param hard_filter_backward_weights  phrasetable weights for filtering backward scores
      * @param combined: if true, use the log-linear sum of both backward and forward
      * score, not just forward, as primary filtering score.
      */
      PhraseInfo4HardFiltering(const pair<const Phrase, TScore>* ref, 
         const Uint numTextTransModels, 
         double log_almost_0, 
         const vector<double>& hard_filter_forward_weights,
         const vector<double>& hard_filter_backward_weights,
         bool combined);
   };

   /// Definition of the Parent inherited
   typedef filterTMVisitor Parent;

   vector<double> hard_filter_forward_weights;  ///< Weights for hard filtering forward scores
   vector<double> hard_filter_backward_weights; ///< Weights for hard filtering backward scores
   bool combined; ///< true iff type was "combined" or "full"

   /**
    * Constructor.
    * @param parent    The parent phrase table, whose getStringPhrase
    *                  function will be used to convert Uint sequence to
    *                  readable, and lexicographically comparable, text.
    * @param log_almost_0
    * @param pruningTypeStr  pruning type: "forward-weights", "backward-weights",
    *                        "combined" or "full"
    * @param forwardWeights  The decoder's forward weights (c.forwardWeights)
    * @param backwardWeights The decoder's backward weights (c.transWeights)
    */
   hardFilterTMVisitor(const PhraseTable &parent, double log_almost_0,
                       const string& pruningTypeStr,
                       vector<double> forwardWeights,
                       const vector<double>& transWeights);

   virtual void operator()(TargetPhraseTable& tgtTable);
};
}; // ends namespace Joint_Filtering
}; // ends namespace Portage

#endif  // __HARD_FILTER_TM_VISITOR_H__

