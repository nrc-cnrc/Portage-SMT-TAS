/**
 * @author Nicola Ueffing
 * @file nbest_posterior_ff.h
 *
 * $Id$
 *
 * N-best Rescoring Module
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada
 *
 * Contains the declaration of all NBest Posterior feature functions
 *   nbestWordPostLev_ff, nbestWordPostSrc_ff, nbestWordPostTrg_ff
 *   nbestPhrasePostSrc_ff, nbestPhrasePostTrg_ff
 *   nbestNgramPost_ff, nbestSentLenPost_ff
 * which are feature functions based on word / phrase / sentence-length
 * posterior probabilities which have been calculated over N-best lists
 */

#ifndef NBESTPOSTERIOR_FF_H
#define NBESTPOSTERIOR_FF_H

#include "featurefunction.h"
#include "nbest_wordpost_lev.h"
#include "nbest_wordpost_src.h"
#include "nbest_wordpost_trg.h"
#include "nbest_phrasepost_src.h"
#include "nbest_phrasepost_trg.h"
#include "nbest_ngrampost.h"
#include "nbest_sentlenpost.h"
#include <vector>
#include <string>

using namespace Portage;

namespace Portage
{
   class nbestPosteriorBase_ff: public FeatureFunction {
      protected:
         NBestPosterior* const nbconf;  ///< Underlying implementation

         double ff_scale;      ///< Mandatory scale factor for the ff weights
         string ff_wts_file;   ///< Mandatory canoe decoder features' and weights
         string ff_prefix;     ///< Optional prefix for features in the wts_file

      private:
         nbestPosteriorBase_ff();

      protected:
         nbestPosteriorBase_ff(const string& args, NBestPosterior* const nbconf);
         virtual bool loadModelsImpl();

      public:
         virtual ~nbestPosteriorBase_ff();
         virtual Uint requires() { return  FF_NEEDS_TGT_TOKENS; }
         virtual void source(Uint s, const Nbest * const nbest);
         virtual bool parseAndCheckArgs();

         virtual double value(Uint k) { return computeValue((*nbest)[k]); }
         virtual double computeValue(const Translation& trans);

         virtual void values(Uint k, vector<double>& vals) { vals = computeValues((*nbest)[k]); }
         virtual vector<double> computeValues(const Translation& trans);
   }; // nbestPosteriorBase_ff

   /**************************** WORD LEV ****************************/
   class nbestWordPostLev_ff: public nbestPosteriorBase_ff {
      public:
         nbestWordPostLev_ff(const string &args);

         virtual FeatureFunction::FF_COMPLEXITY cost() const {
            return HIGH;
         }

         virtual double computeValue(const Translation& trans);
         virtual vector<double> computeValues(const Translation& trans);

   }; // nbestWordPostLev_ff


   /**************************** WORD SRC ****************************/
   class nbestWordPostSrc_ff: public nbestPosteriorBase_ff {
      public:
         nbestWordPostSrc_ff(const string &args);

         virtual Uint requires() {
            return  FF_NEEDS_TGT_TOKENS | FF_NEEDS_ALIGNMENT;
         }

         virtual double computeValue(const Translation& trans);
         virtual vector<double> computeValues(const Translation& trans);

   }; // nbestWordPostSrc_ff


   /**************************** WORD TRG ****************************/
   class nbestWordPostTrg_ff: public nbestPosteriorBase_ff {
      public:
         nbestWordPostTrg_ff(const string &args);

   }; // nbestWordPostTrg_ff



   /**************************** PHRASE SRC ****************************/
   class nbestPhrasePostSrc_ff: public nbestPosteriorBase_ff {
      public:
         nbestPhrasePostSrc_ff(const string &args);

         virtual Uint requires() {
            return  FF_NEEDS_TGT_TOKENS | FF_NEEDS_ALIGNMENT;
         }

         virtual double computeValue(const Translation& trans);
         virtual vector<double> computeValues(const Translation& trans);

   }; // nbestPhrasePostSrc_ff


   /**************************** PHRASE TRG ****************************/
   class nbestPhrasePostTrg_ff: public nbestPosteriorBase_ff {
      public:
         nbestPhrasePostTrg_ff(const string &args);

         virtual Uint requires() {
            return  FF_NEEDS_TGT_TOKENS | FF_NEEDS_ALIGNMENT;
         }

         virtual double computeValue(const Translation& trans);
         virtual vector<double> computeValues(const Translation& trans);

   }; // nbestPhrasePostTrg_ff


   /**************************** N-GRAMS ****************************/
   class nbestNgramPost_ff: public nbestPosteriorBase_ff {
      protected:
         Uint maxN;
      protected:
         virtual bool loadModelsImpl();
      public:
         nbestNgramPost_ff(const string &args);

   }; // nbestSentLenPost_ff


   /**************************** SENTENCE LENGTH ****************************/
   class nbestSentLenPost_ff: public nbestPosteriorBase_ff {
      public:
         nbestSentLenPost_ff(const string &args);

      virtual FeatureFunction::FF_COMPLEXITY cost() const {
         return LOW;
      }

   }; // nbestSentLenPost_ff


} // Portage

#endif // NBESTPOSTERIOR_FF_H
