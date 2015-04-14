/**
 * @author Samuel Larkin
 * @file soft_filter_tm_visitor.h  Interface for soft filtering phrase table
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __SOFT_FILTER_TM_VISITOR_H__
#define __SOFT_FILTER_TM_VISITOR_H__

#include <phrasetable.h>
#include <filter_tm_visitor.h>


namespace Portage {
/// Namespace to hold all relevant code to joint filtering
namespace Joint_Filtering {
/**
 * Callable entity that implements the soft filter_joint per leaf of the trie.
 */
struct softFilterTMVisitor : public filterTMVisitor
{
   /**
   * Yet another PhraseInfo which will be used for soft filter_joint.
   */
   struct PhraseInfo4SoftFiltering : public filterTMVisitor::PhraseInfo4filtering
   {
      /// Definition of the inherited Parent
      typedef filterTMVisitor::PhraseInfo4filtering Parent;

      /**
      * Default constructor.
      * Keeps a pointer to the original object since after filter_joint, we
      * will need the original to rewrite a reduced target table.
      * @param ref pointer to the original object used to construct this
      * @param numTextTransModels  number of text translation models.  We need
      * this information since not all entries have the same size, we need to
      * make them the same size=> the same number of probs for each (forward
      * and/or backward).
      * @param log_almost_0  user defined 0
      */
      PhraseInfo4SoftFiltering(const pair<const Phrase, TScore>* ref, Uint numTextTransModels, double log_almost_0);

   };

   /// Definition of the inherited Parent
   typedef filterTMVisitor Parent;

   /// true iff filtering is in "combined" or "full" mode, i.e., look at all TM models,
   /// not just forward ones.
   bool combined;

   /**
    * Constructor.
    * @param parent    The parent phrase table, whose getStringPhrase
    *                  function will be used to convert Uint sequence to
    *                  readable, and lexicographically comparable, text
    * @param log_almost_0
    * @param pruningTypeStr  pruning type: "forward-weights", "backward-weights",
    *                        "combined" or "full"
    */
   softFilterTMVisitor(const PhraseTable &parent, double log_almost_0, const string& pruningTypeStr);

   virtual void operator()(TargetPhraseTable& tgtTable);

   /**
    * Implements the lessdot from the paper.
    * @param in  left-hand side operand
    * @param jn  right-hand side operand
    * return Returns true if in is less than jn
    */
   bool lessdot(PhraseInfo& in, PhraseInfo& jn) const;

};
}; // ends namespace Joint_Filtering
}; // ends namespace Portage

#endif  // __SOFT_FILTER_TM_VISITOR_H__

