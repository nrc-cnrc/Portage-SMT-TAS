/**
 * @author Samuel Larkin
 * @file phrasetable_filter_grep.h  Interface for filtering phrase table based on source phrases
 *
 * Interface for filtering phrase table based on source phrases
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
          
#ifndef __PHRASE_TABLE_FILTER_GREP_H__
#define __PHRASE_TABLE_FILTER_GREP_H__

#include "phrasetable_filter.h"

namespace Portage {
/**
 * Phrase table for filtering based on input phrases vocabulary
 */
class PhraseTableFilterGrep : public PhraseTableFilter {
   private:   
      /// Definition of PhraseTableFilterLM base class
      typedef PhraseTableFilter Parent;
   protected:
      /// stream to output filtered data
      oMagicStream  os_filter_output;

   public:
      /**
       * Default constructor.
       * @param _limitPhrases       limiting 
       * @param tgtVocab            target vocaulary
       * @param pruningTypeStr      pruning type
       */
      PhraseTableFilterGrep(bool _limitPhrases, VocabFilter& tgtVocab,
                            const string& pruningTypeStr,
                            bool appendJointCounts = false);

      /// Destructor.
      virtual ~PhraseTableFilterGrep();
      
      virtual bool processEntry(PhraseTableEntry& entry);
      virtual Uint filter(const string& TM_filename, const string& filtered_TM_filename);

}; // ends class PhraseTableFilterGrep
}; // ends namespace Portage

#endif  // __PHRASE_TABLE_FILTER_GREP_H__
