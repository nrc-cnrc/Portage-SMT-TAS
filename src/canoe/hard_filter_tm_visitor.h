/**
 * @author Samuel Larkin
 * @file hard_filter_tm_visitor.h  Defines the hard filter visitor for phrase tables
 *
 * $Id$
 *
 * Defines the hard filter visitor for filtering phrase tables based on
 * filter_joint algorithm
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
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
      * @param hard_filter_weights  phrasetable weights
      */
      PhraseInfo4HardFiltering(const pair<Phrase, TScore>* ref, 
         const Uint numTextTransModels, 
         double log_almost_0, 
         const vector<double>& hard_filter_weights);
   };

   /// Definition of the Parent inherited
   typedef filterTMVisitor Parent;

   const vector<double>* const hard_filter_weights;  ///< TM weights for hard filtering

   /**
    * Constructor.
    * @param parent    The parent phrase table, whose getStringPhrase
    *                  function will be used to convert Uint sequence to
    *                  readable, and lexicographically comparable, text
    * this information since not all entries have the same size, we need to
    * make them the same size=> the same number of probs for each (forward
    * and/or backward).
    * @param log_almost_0
    * @param hard_filter_weights
    */
   hardFilterTMVisitor(const PhraseTable &parent, double log_almost_0, const vector<double>* const hard_filter_weights);

   virtual void operator()(TargetPhraseTable& tgtTable);
};
}; // ends namespace Joint_Filtering
}; // ends namespace Portage

#endif  // __HARD_FILTER_TM_VISITOR_H__

