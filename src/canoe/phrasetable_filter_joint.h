/**
 * @author Samuel Larkin
 * @file phrasetable_filter_joint.h  Interface for filtering phrase table based on filter_joint
 *
 * $Id$
 *
 * Interface for filtering phrase table based on filter_joint
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 */
          
#ifndef __PHRASE_TABLE_FILTER_JOINT_H__
#define __PHRASE_TABLE_FILTER_JOINT_H__

#include <phrasetable_filter.h>
#include <filter_tm_visitor.h>


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
      Joint_Filtering::filterTMVisitor*   visitor;
      TargetPhraseTable* tgtTable;                 ///< Use for complete online filtering (save trie's query)
      OMagicStream*      online_filter_output;     ///< output file name when processing online
      Uint               L;                        ///< number of entries, limit for filtering

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
      ~PhraseTableFilterJoint();

      /**
       * Sets the output filename for online processing.
       * @param  filename  output filename
       * @param  L         limit
       */
      void outputForOnlineProcessing(const string& filename, Uint L);

      virtual float convertFromRead(const float& value) const;
      virtual float convertToWrite(const float& value) const;
      virtual void  filter_joint(const string& filename, Uint L = 30);
      virtual Uint  processTargetPhraseTable(const string& src, TargetPhraseTable* tgtTable);
      virtual TargetPhraseTable* getTargetPhraseTable(const Entry& entry, bool limitPhrases);

}; // ends class PhraseTableFilterJoint
}; // ends namespace Portage

#endif  // __PHRASE_TABLE_FILTER_JOINT_H__

