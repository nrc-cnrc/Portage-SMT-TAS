/**
 * @author Nicola Ueffing
 * @file mismatch_ff.h  Feature Function that calculates mismatch of punctuation marks
 * 
 * 
 * COMMENTS: 
 *
 * Mismatch of punctuation marks.
 *   ParMismatch:  mismatch between opening and closing brackets within a hypothesis
 *   QuotMismatch: mismatch between of quotation marks between source and hypothesis
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef MISMATCH_FF_H
#define MISMATCH_FF_H

#include "featurefunction.h"

namespace Portage 
{

/// Feature Function that calculates a mismatch between opening and closing brackets
class ParMismatchFF : public FeatureFunction
{
   private:
      /// Scores for one nbest list.
      vector<double> scores;

   public:
      /// Constructor.
      /// @param dontcare  just to be compatible with all other feature function.
      ParMismatchFF(const string& dontcare);

      virtual bool parseAndCheckArgs();

      virtual Uint requires() { return FF_NEEDS_TGT_TOKENS; }

      virtual FeatureFunction::FF_COMPLEXITY cost() const { return LOW; }

      virtual void source(Uint s, const Nbest * const nbest);

      virtual double value(Uint k) { return scores[k]; }
};
  
/// Feature Function that calculates a mismatch between quotation marks in source and target
class QuotMismatchFF : public FeatureFunction
{
   private:
      /// Scores for one nbest list.
      vector<double> scores;
      char srclang, tgtlang;

      /* Determines numbers of different types of quotation marks
       * @param sent     sentence 
       * @param lang     language
       * @param quo      number of opening quotes
       * @param quc      number of closing quotes
       * @param qun      number of neutral quotes
       */
      void findQuot(const Tokens& sent, const char lang, int &quo, int &quc, int &qun);

   public:
      /// Constructor.
      /// @param args  arguments specifying from and to languages.
      QuotMismatchFF(const string& args);

      virtual bool parseAndCheckArgs();

      virtual Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }

      virtual void source(Uint s, const Nbest * const nbest);

      virtual double value(Uint k) { return scores[k]; }
};
  
}

#endif 
