/**
 * @author Samuel Larkin
 * @file filter_tm_visitor.h  Generic interface for filtering phrase table
 *                            based on filter_joint
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

#ifndef __FILTER_TM_VISITOR_H__
#define __FILTER_TM_VISITOR_H__

#include "phrasetable.h"
#include "simple_histogram.h"
#include "pruning_style.h"


namespace Portage {
/// Namespace to hold all relevant code to joint filtering
namespace Joint_Filtering {
/**
 * Interface of a callable entity for soft and hard filtering of phrase tables
 */
struct filterTMVisitor : private NonCopyable
{
   /**
   * Yet another PhraseInfo which will be used for filter_joint.
   */
   struct PhraseInfo4filtering : public ForwardBackwardPhraseInfo
   {
      /// holds a pointer to the entry this was constructed from to later,
      /// during filter_joint, rewrite the original object.
      const pair<const Phrase, TScore>* ref;

      /**
      * Default constructor.
      * Keeps a pointer to the original object since after filter_joint, we
      * will need the original to rewrite a reduced target table.
      * @param ref  reference on the original object used to construct this
      */
      PhraseInfo4filtering(const pair<const Phrase, TScore>* ref)
      : ref(ref)
      { assert(ref); }

      /**
      * Converts probs to their log values.
      * @param  convertedProbs  returned log probs
      * @param  originalProbs   orignal probs
      * @param numTextTransModels  number of text translation models.  We need
      *   this information since not all entries have the same size, we need to
      *   make them the same size=> the same number of probs for each (forward
      *   and/or backward).
      * @param log_almost_0  user defined 0
      */
      void MakeLogProbs(vector<float>& convertedProbs, const vector<float>& originalProbs,
                        Uint numTextTransModels, double log_almost_0) const
      {
         // EJJ Nov 2010: the following assertion is no longer appropropriate,
         // since we now allow multi prob tables with empty 3rd columns.
         //assert(originalProbs.size() > 0);
         convertedProbs.resize(numTextTransModels, log_almost_0);
         for (Uint i(0); i<originalProbs.size(); ++i) {
            if (originalProbs[i] <= 0.0f)
               convertedProbs[i] = log_almost_0;
            else
               convertedProbs[i] = log(originalProbs[i]);
         }
      }

      /**
      * Prints the content of this for debugging purpose.
      * @param index  since this refers to one entry in the target table, we print its index
      */
      void print(Uint index) const {
         cerr << index << " | ";
         copy(forward_trans_probs.begin(), forward_trans_probs.end(), ostream_iterator<double>(cerr, " "));
         cerr << "| ";
         copy(phrase_trans_probs.begin(), phrase_trans_probs.end(), ostream_iterator<double>(cerr, " "));
         cerr << "| ";
         cerr << forward_trans_prob << " | " << phrase_trans_prob << " | ";
         copy(phrase.begin(), phrase.end(), ostream_iterator<Uint>(cerr, " "));
         cerr << endl;
      }
   };

   const PhraseTable &parent;          ///< parent PhraseTable object
   Uint L;                             ///< filtering length
   const PhraseTable::PhraseScorePairLessThan phraseLessThan;  ///< to order item for dynamic filtering algo
   const PhraseTable::PhraseScorePairGreaterThan phraseGreaterThan;  ///< to order item for dynamic filtering algo
   Uint numTextTransModels;              ///< number of text translation models, known after all models were loaded
   const double log_almost_0;            ///< log(0), almost
   const char* const style;              ///< Should indicate hard or soft filtering
   Uint numKeptEntry;                    ///< Keeps track of how many entries were kept

   /// reference on the pruning type.
   const pruningStyle* pruning_type;

   /// Gathers stats on number leaves and number of entries per leaves.
   SimpleHistogram<Uint>* stats_unfiltered;
   /// Gathers stats on number leaves and number of entries per leaves.
   SimpleHistogram<Uint>* stats_filtered;

   /**
    * Default constructor.
    * @param parent        phrase table from which the leaf is filtered from.
    * @param log_almost_0  value of what will be considered log(-0)
    * @param style         a string representing the derived class type for generic msg printing
    */
   filterTMVisitor(const PhraseTable &parent, double log_almost_0, const char* const style)
      : parent(parent)
      , L(0)
      , phraseLessThan(parent)
      , phraseGreaterThan(parent)
      , numTextTransModels(0)
      , log_almost_0(log_almost_0)
      , style(style)
      , numKeptEntry(0)
      , pruning_type(NULL)
      , stats_unfiltered(NULL)
      , stats_filtered(NULL)
   {}

   virtual ~filterTMVisitor()
   {
      if (stats_unfiltered) delete stats_unfiltered;
      if (stats_filtered) delete stats_filtered;
   }

   /**
    * Simply sets the pruning style and the number of text translation models.
    * @param _pruning_type  pruning style.
    * @param n      number of text translation models.
    */
   void set(const pruningStyle* const _pruning_type, Uint n) 
   {
      pruning_type = _pruning_type;
      set(Uint(0), n);
   }

   /**
    * Simply sets the limit and the number of text translation models.
    * @param Limit  filtering length
    * @param n      number of text translation models.
    */
   void set(Uint Limit, Uint n)
   {
      L = Limit;
      numTextTransModels = n;

      if (stats_unfiltered) delete stats_unfiltered;
      stats_unfiltered = new SimpleHistogram<Uint>(new fixBinner(300));
      assert(stats_unfiltered);

      if (stats_filtered) delete stats_filtered;
      stats_filtered = new SimpleHistogram<Uint>(new fixBinner(30));
      assert(stats_filtered);
   }

   /**
    * Displays the histogram values of before filtering and after filtering.
    * @param  out  where to output the histograms
    */
   void display(ostream& out) const
   {
      out << "Histogram before filtering" << endl;
      stats_unfiltered->display(out, "  ");
      out << "Histogram after filtering" << endl;
      stats_filtered->display(out, "  ");
   }

   /**
    * Transforms the phraseTable in a visitor to filter "30" the trie leaves.
    * Gets called by trie_node.traverse.
    * @param key_stack  representation of the trie key
    * @param tgtTable   leaf of the trie to filter
    */
   void operator()(const vector<Uint>& key_stack, TargetPhraseTable& tgtTable)
   {
      // There is no need to process an empty leaf.
      if (tgtTable.empty()) return;

      // DEBUGGING
      //copy(key_stack.begin(), key_stack.end(), ostream_iterator<Uint>(cerr, " ")); cerr << endl;

      // Gather stats of unfiltered leaves
      stats_unfiltered->add(tgtTable.size());

      // Calculate the proper L for the given source phrase.
      if (pruning_type != NULL) {
         L = (*pruning_type)(key_stack.size());
      }

      operator()(tgtTable);
      numKeptEntry += tgtTable.size();

      // Gather stats of filtered leaves
      stats_filtered->add(tgtTable.size());
   }

   /**
    * Transforms the phraseTable in a visitor to filter "30" the trie leaves.
    * @param tgtTable   leaf of the trie to filter
    */
   virtual void operator()(TargetPhraseTable& tgtTable) = 0;
};
}; // ends namespace Joint_Filtering
}; // ends namespace Portage

#endif  // __FILTER_TM_VISITOR_H__

