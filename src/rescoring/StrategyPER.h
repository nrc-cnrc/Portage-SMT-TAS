/**
 * @author Samuel Larkin
 * @file StrategyPER.h  Encapsulation to make BLEU WER and PER similaire
 * enough to make them interchangeable.
 *
 *
 * COMMENTS:
 *
 * Groupe de technologies langagières interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Conseil national de recherches du Canada / Copyright 2005, National Research Council of Canada
 */
#ifndef __STRATEGY_PER_H__
#define __STRATEGY_PER_H__

#include <string>
#include <wer.h>
#include <str_utils.h>

namespace Portage
{
   /// Encapsulation to make BLEU WER and PER similaire enough to make them
   /// interchangeable.
   struct StrategyPER
   {
      /// Definition of a metric in the context of PER
      typedef double metrique;
      /// Metric's value
      metrique info;

      /**
       * Calculates PER score given a target and a reference
       */
      void compute(const string& tgt, const string& refs)
      {
         vector<string> toksTgt;
         vector<string> toksRef;
         split(tgt, toksTgt);
         split(refs, toksRef);

         info = find_mPER(toksTgt, toksRef);
      }

      /// Get the metric's score so it can be minimized.
      /// @return Returns the metric's score.
      double score()
      {
         return info;
      }
   };
};

#endif   // __STRATEGY_PER_H__
