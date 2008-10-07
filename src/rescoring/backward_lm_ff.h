/**
 * @author Samuel Larkin
 * @file backward_lm_ff.h  Feature function for backward lm
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l.information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef __BACKWARD_LM_FF_H__
#define __BACKWARD_LM_FF_H__

#include "featurefunction.h"


namespace Portage
{
   class PLM;
   class Voc;

   /// Feature function that scores a hypothesis based on a backward
   /// language model word probability.
   class BackwardLmFF: public FeatureFunction
   {
      private:
         PLM* lm;     ///< Language model.
         Voc::indexConverter* converter;  ///< Object use to convert string to uint tgt_voc 

      protected:
         virtual bool parseAndCheckArgs();

      public:
         /// Constructor.
         /// @param args  arguments to construct a backward language model.
         BackwardLmFF(const string &args);
         /// Destructor.
         ~BackwardLmFF();

         virtual void init(const Sentences * const src_sents);
         virtual Uint requires() { return FF_NEEDS_TGT_TOKENS | FF_NEEDS_TGT_VOCAB; }
         virtual double value(Uint k);
         virtual void values(Uint k, vector<double>& vals);
         virtual void addTgtVocab(VocabFilter *_tgt_vocab);
   };
}

#endif // __BACKWARD_LM_FF_H__
