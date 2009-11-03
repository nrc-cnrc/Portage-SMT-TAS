/**
 * @author Samuel Larkin
 * @file StrategyBLEU.h  Encapsulation to make BLEU WER and PER similaire
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
#ifndef __STRATEGY_BLEU_H__
#define __STRATEGY_BLEU_H__

#include <string>
#include "bleu.h"
#include <math.h>

namespace Portage
{
   /// Encapsulation to make BLEU WER and PER similaire enough to make them
   /// interchangeable.
   struct StrategyBLEU
   {
      /// Definition of a metric in the context of BLEU
      typedef BLEUstats metrique;
      /// Metric's value
      metrique info;

      /**
       * Calculates BLEU score given a target and a reference
       */
      void compute(const string& tgt, const string& refs)
      {
         vector<string> references;
         references.push_back(refs);
         info = BLEUstats(tgt, references, 1);
      }

      /// Get the metric's score so it can be minimized.
      /// @return Returns the metric's score.
      double score()
      {
         //return -exp(info.score());
         //return 1-exp(info.score());
         return -info.score();
      }
   };
}

#endif   // __STRATEGY_BLEU_H__
