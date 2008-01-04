/**
 * @author Aaron Tikuisis
 * @file lm_ff.h  Feature function ngram
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

#ifndef LM_FF_H
#define LM_FF_H

#include "featurefunction.h"


namespace Portage
{
   class PLM;
   class Voc;

   /// Feature function that scores a hypothesis based on the language model
   /// word probability.
   class NgramFF: public FeatureFunction
   {
      private:
         PLM *lm;     ///< Language model.

      protected:
         virtual bool parseAndCheckArgs();

      public:
         /// Constructor.
         /// @param args  arguments to construct a language model.
         NgramFF(const string &args);
         /// Destructor.
         ~NgramFF();

         virtual void init(const Sentences * const src_sents);
         virtual Uint requires() { return FF_NEEDS_TGT_TOKENS | FF_NEEDS_TGT_VOCAB; }
         virtual double value(Uint k);
         virtual void values(Uint k, vector<double>& vals);
   };
}

#endif // LM_FF_H
