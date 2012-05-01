/**
 * @author Samuel Larkin
 * @file phrasetable_filter_joint.h  Interface for filtering phrase table based on filter_joint
 *
 * $Id$
 *
 * Interface for filtering phrase table based on filter_joint
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */

#ifndef __PHRASE_TABLE_FILTER_JOINT_H__
#define __PHRASE_TABLE_FILTER_JOINT_H__

#include "phrasetable_filter.h"
#include "filter_tm_visitor.h"


namespace Portage {
/**
 * Phrase table for filtering using the filter joint algorithm
 */
class PhraseTableFilterJoint : public PhraseTableFilter {
   private:
      /// Definition of PhraseTableFilterLM base class
      typedef PhraseTableFilter Parent;

   protected:
      /// Performs either hard or soft filtering
      Joint_Filtering::filterTMVisitor* visitor;

      /// Use for complete online filtering (save trie's query)
      TargetPhraseTable* tgtTable;

      const pruningStyle* const pruning_type;

      /// output file name when processing online
      oSafeMagicStream* online_filter_output;

   public:
      /**
       * Default constructor.
       * @param limitPhrases        limiting
       * @param tgtVocab            target vocaulary
       * @param pruning_type
       * @param pruningTypeStr      pruning type
       * @param hard_filter_weights             TM weights for hard filtering
       */
      PhraseTableFilterJoint(bool limitPhrases,
         VocabFilter& tgtVocab,
         const pruningStyle* const pruning_type,
         const char* pruningTypeStr = NULL,
         const vector<double>* const hard_filter_weights = NULL);

      /// Destructor.
      virtual ~PhraseTableFilterJoint();

      /**
       * Once all phrase tables have been loaded, apply the filter_joint
       * algorithm and dump multiprobs to filename.
       * @param filtered_TM_filename  file name to store the filtered TM.
       */
      virtual void filter(const string& filtered_TM_filename);

      /**
       * Filters without keeping entry in memory.
       * @param TM_filename  multi probs input file name
       * @param filtered_TM_filename  multi probs output file name.
       */
      virtual Uint filter(const string& TM_filename, const string& filtered_TM_filename) {
         outputForOnlineProcessing(filtered_TM_filename);
         return PhraseTable::readMultiProb(TM_filename.c_str(), limitPhrases);
      }

      /**
       * Sets the output filename for online processing.
       * @param  filtered_TM_filename  output filename
       */
      void outputForOnlineProcessing(const string& filtered_TM_filename);

      virtual float convertFromRead(float value) const;
      virtual float convertToWrite(float value) const;

      virtual Uint processTargetPhraseTable(const string& src, Uint src_word_count, TargetPhraseTable* tgtTable);
      virtual TargetPhraseTable* getTargetPhraseTable(Entry& entry, bool limitPhrases);

}; // ends class PhraseTableFilterJoint
}; // ends namespace Portage

#endif  // __PHRASE_TABLE_FILTER_JOINT_H__

