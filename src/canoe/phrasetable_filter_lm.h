/**
 * @author Samuel Larkin
 * @file phrasetable_filter_lm.h  Interface for phrase table when filtering LMs.
 *
 * Interface for phrase table when filtering LMs.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */
          
#ifndef __PHRASE_TABLE_FILTER_LM_H__
#define __PHRASE_TABLE_FILTER_LM_H__

#include "phrasetable_filter.h"

namespace Portage {
/**
 * Phrase table for filtering LMs.
 * This class doesn't create in memory the phrase table trie but rather takes
 * care of loading the tgt_vocab used for filtering the LMs.
 */
class PhraseTableFilterLM : public PhraseTableFilter {
   private:
      /// Definition of PhraseTableFilterLM base class
      typedef PhraseTableFilter Parent;
   public:
      /**
       * Default constructor.
       * @param _limitPhrases       limiting 
       * @param tgtVocab            target vocaulary
       * @param pruningTypeStr      pruning type
       */
      PhraseTableFilterLM(bool _limitPhrases, VocabFilter& tgtVocab,
                          const string& pruningTypeStr,
                          bool appendJointCounts = false);

      /// Destructor.
      virtual ~PhraseTableFilterLM();

      virtual bool processEntry(PhraseTableEntry& entry);

}; // ends class PhraseTableFilterLM
} // ends namespace Portage

#endif  // __PHRASE_TABLE_FILTER_LM_H__
