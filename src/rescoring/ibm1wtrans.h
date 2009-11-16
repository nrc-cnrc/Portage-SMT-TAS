/**
 * @author Aaron Tikuisis
 * @file ibm1wtrans.h  IBM1WTransTgtGivenSrc and IBM1WTransSrcGivenTgt:
 *                     features that check that all words were translated, and
 *                     that no words were inserted, respectively.
 *
 * $Id$
 * 
 * K-best Rescoring Module
 *
 * Note: this feature was formerly known as IBM1Aaron{TgtGivenSrc,SrcGivenTgt}.
 * 
 * Contains the declaration of IBM1WTransTgtGivenSrc, which is a feature
 * function defined as follows:
 * \f$ \max_{f:\mbox{src\_toks} \rightarrow \mbox{tgt\_toks} (1-1)} \sum_{s \in \mbox{src\_toks}}
 *	( \frac{P(f(s)|s)}{max_{t \in \mbox{tgt\_vocab}} P(t|s)} ) \f$
 * Also contains IBM1WTransSrcGivenTgt, which is the same except with source and
 * target completely reversed.
 *
 * Other variations that haven't been implemented are:
 * - Reverse conditional direction in probabilities
 * - Take maximum over all functions instead of (1-1) ones
 *
 * The idea is to have a sort of measure of how many words are well-translated.
 * \f$ \frac{P(t|s)}{max_{t' \in \mbox{tgt\_vocab}} P(t'|s)}\f$ is a sort of
 * measure of whether t is a good translation of s.
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 */

#ifndef IBM1WTRANS_FF_H
#define IBM1WTRANS_FF_H

#include <featurefunction.h>
#include <ttablewithmax.h>
#include <vector>
#include <string>

using namespace Portage;

namespace Portage
{
  /// Interface for IBM1 WTrans.
  class IBM1WTransBase: public FeatureFunction {
    private:
      TTableWithMax*  table;
    protected:
      /**
       *
       * @param src  source tokens
       * @param tgt  target tokens
       * @return Returns
       */
      double computeValue(const Tokens& src, const Tokens& tgt);
      virtual bool parseAndCheckArgs();
      virtual bool loadModelsImpl();
    
    public:
      /// Constructor.
      /// @param file  file and arguments
      IBM1WTransBase(const string &file);
      /// Destructor
      virtual ~IBM1WTransBase();
    
      virtual Uint requires() { return  FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
  };

  /// Forward IBM1WTrans.
  class IBM1WTransTgtGivenSrc: public IBM1WTransBase {
    public:
      /// Constructor.
      /// @param file  file and arguments
      IBM1WTransTgtGivenSrc(const string &file) : IBM1WTransBase(file) {}

      virtual double value(Uint k) {
        return computeValue((*src_sents)[s].getTokens(), (*nbest)[k].getTokens());
      }
  }; // IBM1WTransTgtGivenSrc
    
  /// Backward IBM1WTrans.
  class IBM1WTransSrcGivenTgt: public IBM1WTransBase {
    public:
      /// Constructor.
      /// @param file  file and arguments
      IBM1WTransSrcGivenTgt(const string &file) : IBM1WTransBase(file) {}
	    
      virtual double value(Uint k) {
        return computeValue((*nbest)[k].getTokens(), (*src_sents)[s].getTokens());
      }
  }; // IBM1WTransSrcGivenTgt
} // Portage

#endif // IBM1WTRANS_FF_H
