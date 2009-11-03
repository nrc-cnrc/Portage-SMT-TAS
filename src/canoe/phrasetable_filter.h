/**
 * @author Samuel Larkin
 * @file phrasetable_filter.h  Abstract class for a filtering phrase table
 *
 * $Id$
 *
 * Abstract class for a filtering phrase table 
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
          
#ifndef __PHRASE_TABLE_FILTER_H__
#define __PHRASE_TABLE_FILTER_H__

#include "phrasetable.h"
#include "pruning_style.h"

namespace Portage {
/**
 * Abstract class for filtering phrase tables
 */
class PhraseTableFilter : public PhraseTable {
   private:   
      /// Definition of PhraseTableFilter base class
      typedef PhraseTable Parent;
   protected:
      /// Are we limiting phrase table to the input phrase vocabulary?
      bool limitPhrases;
      /// Keeps a reference to a pruning Style.
      const pruningStyle* pruning_type;
   public:
      /**
       * Default constructor.
       * @param limitPhrases        limiting 
       * @param tgtVocab            target vocaulary
       * @param pruningTypeStr      pruning type
       */
      PhraseTableFilter(bool limitPhrases, VocabFilter& tgtVocab, const char* pruningTypeStr = NULL)
      : Parent(tgtVocab, pruningTypeStr)
      , limitPhrases(limitPhrases)
      , pruning_type(NULL)
      {}

      /// Destructor.
      virtual ~PhraseTableFilter()
      {}


      /**
       * Once all phrase tables have been loaded, apply the filter_joint
       * algorithm and dump multiprobs to filename.
       * @param filename  file name of the resulting multi probs
       * @param L  ttable-limit
       */
      virtual void filter_joint(const string& filename, const pruningStyle* const pruning_type)
      {}

      /**
       * Takes care of single probs.
       * @param src_given_tgt_file  backward input file
       * @param backward_output     backward output file
       * @param tgt_given_src_file  forward input file
       * @param forward_output      forward output file
       */
      virtual void processSingleProb(const string& src_given_tgt_file, 
         const string& backward_output,
         const char *tgt_given_src_file = NULL,
         const char* forward_output = NULL)
      { Parent::read(src_given_tgt_file.c_str(), tgt_given_src_file, limitPhrases); }   

      /**
       * Takes care of multi probs.
       * @param multi_prob_TM_filename  multi probs input file name
       * @param filtered_output  multi probs output file name.
       */
      virtual Uint processMultiProb(const string& multi_prob_TM_filename, const string& filtered_output)
      { return Parent::readMultiProb(multi_prob_TM_filename.c_str(), limitPhrases); }

}; // ends class PhraseTableFilter
}; // ends namespace Portage

#endif // __PHRASE_TABLE_FILTER_H__
