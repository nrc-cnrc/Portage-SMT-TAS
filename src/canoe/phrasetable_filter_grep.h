/**
 * @author Samuel Larkin
 * @file phrasetable_filter_grep.h  Interface for filtering phrase table based on source phrases
 *
 * $Id$
 *
 * Interface for filtering phrase table based on source phrases
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
          
#ifndef __PHRASE_TABLE_FILTER_GREP_H__
#define __PHRASE_TABLE_FILTER_GREP_H__

#include <phrasetable_filter.h>

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
      PhraseTableFilterGrep(bool _limitPhrases, VocabFilter& tgtVocab, const char* pruningTypeStr = NULL);
      /// Destructor.
      ~PhraseTableFilterGrep();
      
      /**
       * Given a phrase table in a file, filters it and output the filtered results to an another file.
       * @param input     file name of the phrase table to be filtered
       * @param output    file name of the filtered phrase table
       * @param reversed  indicates if the phrase table's direction is reversed
       */
      void filterSingleProb(const string& input, const string& output, const bool reversed);

      virtual bool processEntry(TargetPhraseTable* tgtTable, Entry& entry);
      virtual void processSingleProb(const string& src_given_tgt_file, 
         const string& backward_output,
         const char *tgt_given_src_file = NULL,
         const char* forward_output = NULL);
      virtual Uint processMultiProb(const string& multi_prob_TM_filename, const string& filtered_output);

}; // ends class PhraseTableFilterGrep
}; // ends namespace Portage

#endif  // __PHRASE_TABLE_FILTER_GREP_H__
