/**
 * @author Eric Joanis
 * @file hmm_ff.h  Feature functions based on HMM aligner, in both directions.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef HMM_FF_H
#define HMM_FF_H

#include <hmm_aligner.h>
#include "featurefunction.h"

// Note that all methods are in the .h in the class, partially out of laziness,
// but also because this file is only included by featurefunction_set.cc, so no
// compilation overhead is incurred.
namespace Portage {

/// Forward HMM
class HMMTgtGivenSrc : public FeatureFunction
{
   protected:
      HMMAligner* hmmaligner;

   protected:
      virtual bool parseAndCheckArgs() {
         if (argument.empty()) {
            error(ETWarn, "You must provide a HMM-aligner forward probability file to HMMTgtGivenSrc");
            return false;
         }

         if (!check_if_exists(argument)){
            error(ETWarn, "File is not accessible: %s", argument.c_str());
            return false;
         }

         return true;
      }
      virtual bool loadModelsImpl() {
         hmmaligner = new HMMAligner(argument);
         if ( !hmmaligner )
            error(ETWarn, "Failed to create HMM Aligner with model %s.",
                  argument.c_str());
         return hmmaligner != NULL;
      }

   public:

      /**
       * Constructor, the model should have been trained for p(tgt|src).
       * @param ttable_file  file containing the ttable.
       */
      HMMTgtGivenSrc(const string& ttable_file)
         : FeatureFunction(ttable_file)
         , hmmaligner(NULL)
      {}
      virtual ~HMMTgtGivenSrc() {
         if (hmmaligner) delete hmmaligner, hmmaligner = NULL;
      }

      virtual Uint requires() {
         return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS;
      }
      virtual double value(Uint k) {
         assert(hmmaligner);
         return hmmaligner->logpr((*src_sents)[s].getTokens(),      
               (*nbest)[k].getTokens());
      }
};

/// Backward HMM
class HMMSrcGivenTgt : public FeatureFunction
{
   protected:
      HMMAligner* hmmaligner;

   protected:
      virtual bool parseAndCheckArgs() {
         if (argument.empty()) {
            error(ETWarn, "You must provide a HMM-aligner backward probability file to HMMSrcGivenTgt");
            return false;
         }

         if (!check_if_exists(argument)){
            error(ETWarn, "File is not accessible: %s", argument.c_str());
            return false;
         }

         return true;
      }
      virtual bool loadModelsImpl() {
         hmmaligner = new HMMAligner(argument);
         if ( !hmmaligner )
            error(ETWarn, "Failed to create HMM Aligner with model %s.",
                  argument.c_str());
         return hmmaligner != NULL;
      }

   public:

      /**
       * Constructor, the model should have been trained for p(src|tgt).
       * @param ttable_file  file containing the ttable.
       */
      HMMSrcGivenTgt(const string& ttable_file) 
         : FeatureFunction(ttable_file)
         , hmmaligner(NULL)
      {}
      virtual ~HMMSrcGivenTgt() {
         if (hmmaligner) delete hmmaligner, hmmaligner = NULL;
      }

      virtual Uint requires() {
         return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS;
      }
      virtual FeatureFunction::FF_COMPLEXITY cost() const {
         return HIGH;
      }
      virtual double value(Uint k) {
         assert(hmmaligner);
         return hmmaligner->logpr((*nbest)[k].getTokens(),
               (*src_sents)[s].getTokens());
      }
};

/// Forward HMM aligner using Viterbi alignments
class HMMVitTgtGivenSrc : public HMMTgtGivenSrc {
   public:
      HMMVitTgtGivenSrc(const string& ttable_file) : HMMTgtGivenSrc(ttable_file) {}
      virtual double value(Uint k) {
         assert(hmmaligner);
         return hmmaligner->viterbi_logpr((*src_sents)[s].getTokens(),
               (*nbest)[k].getTokens());
      }
};

/// Backward HMM aligner using Viterbi alignments
class HMMVitSrcGivenTgt : public HMMSrcGivenTgt {
   public:
      HMMVitSrcGivenTgt(const string& ttable_file) : HMMSrcGivenTgt(ttable_file) {}
      virtual double value(Uint k) {
         assert(hmmaligner);
         return hmmaligner->viterbi_logpr((*nbest)[k].getTokens(),
               (*src_sents)[s].getTokens());
      }
};

} // Portage

#endif // HMM_FF_H

