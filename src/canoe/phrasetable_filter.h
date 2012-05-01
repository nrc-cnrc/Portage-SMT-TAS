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
       * Takes care of multi probs.
       * @param TM_filename  multi probs input file name
       * @param filtered_TM_filename  multi probs output file name.
       */
      virtual Uint filter(const string& TM_filename, const string& filtered_TM_filename)
      { return Parent::readMultiProb(TM_filename.c_str(), limitPhrases); }

      Uint readMultiProb(const string& filename) 
      { return Parent::readMultiProb(filename.c_str(), limitPhrases); }

      virtual void addSourceSentences(const vector<vector<string> >& sentences) {
         if (limitPhrases)
            Parent::addSourceSentences(sentences);
      }
}; // ends class PhraseTableFilter
}; // ends namespace Portage

#endif // __PHRASE_TABLE_FILTER_H__
