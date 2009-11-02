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

      /// output file name when processing online
      oSafeMagicStream* online_filter_output;

   public:
      /**
       * Default constructor.
       * @param limitPhrases        limiting
       * @param tgtVocab            target vocaulary
       * @param pruningTypeStr      pruning type
       * @param hard_filter_weights             TM weights for hard filtering
       */
      PhraseTableFilterJoint(bool limitPhrases,
         VocabFilter& tgtVocab,
         const char* pruningTypeStr = NULL,
         const vector<double>* const hard_filter_weights = NULL);

      /// Destructor.
      virtual ~PhraseTableFilterJoint();

      /**
       * Sets the output filename for online processing.
       * @param  filename  output filename
       * @param  pruning_type
       */
      void outputForOnlineProcessing(const string& filename, const pruningStyle* pruning_type);

      virtual float convertFromRead(float value) const;
      virtual float convertToWrite(float value) const;
      virtual void  filter_joint(const string& filename, const pruningStyle* const pruning_type);
      virtual Uint  processTargetPhraseTable(const string& src, Uint src_word_count, TargetPhraseTable* tgtTable);
      virtual TargetPhraseTable* getTargetPhraseTable(const Entry& entry, Uint src_word_count, bool limitPhrases);

}; // ends class PhraseTableFilterJoint
}; // ends namespace Portage

#endif  // __PHRASE_TABLE_FILTER_JOINT_H__

