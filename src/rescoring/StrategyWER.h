/**
 * @author Samuel Larkin
 * @file StrategyWER.h  Encapsulation to make BLEU WER and PER similaire
 * enough to make them interchangeable.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#ifndef __STRATEGY_WER_H__
#define __STRATEGY_WER_H__

#include <string>
#include <wer.h>
#include <str_utils.h>

namespace Portage
{
   /// Encapsulation to make BLEU WER and PER similaire enough to make them
   /// interchangeable.
   struct StrategyWER
   {
      /// Definition of a metric in the context of WER
      typedef double metrique;
      /// Metric's value
      metrique info;

      /**
       * Calculates WER score given a target and a reference
       */
      void compute(const string& tgt, const string& refs)
      {
         vector<string> toksTgt;
         vector<string> toksRef;
         split(tgt, toksTgt);
         split(refs, toksRef);

         info = find_mWER(toksTgt, toksRef);
      }

      /// Get the metric's score so it can be minimized.
      /// @return Returns the metric's score.
      double score()
      {
         return info;
      }
   };
}

#endif   // __STRATEGY_WER_H__
