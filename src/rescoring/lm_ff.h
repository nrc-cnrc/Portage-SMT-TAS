/**
 * @author Aaron Tikuisis
 * @file lm_ff.h  Feature function ngram
 *
 * $Id$
 *
 * K-Best Rescoring Module
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group 
 * Institut de technologie de l.information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
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
      Voc *vocab;  ///< vocabulary.
      PLM *lm;     ///< Language model.
   public:
      /// Constructor.
      /// @param args  arguments to construct a language model.
      NgramFF(const string &args);
      /// Destructor.
      ~NgramFF();

      virtual inline Uint requires() { return FF_NEEDS_TGT_TOKENS; }
      virtual double value(Uint k);
};
}

#endif // LM_FF_H
