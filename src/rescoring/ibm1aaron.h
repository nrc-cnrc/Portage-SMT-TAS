/**
 * @author Aaron Tikuisis
 * @file ibm1aaron.h  Contains the declaration of IBM1AaronTgtGivenSrc.
 *
 * $Id$
 * 
 * K-best Rescoring Module
 * 
 * Contains the declaration of IBM1AaronTgtGivenSrc, which is a feature function
 * defined as follows:
 * \f$ \max_{f:\mbox{src\_toks} \rightarrow \mbox{tgt\_toks} (1-1)} \sum_{s \in \mbox{src\_toks}}
 *	( \frac{P(f(s)|s)}{max_{t \in \mbox{tgt\_vocab}} P(t|s)} ) \f$
 * Also contains IBM1AaronSrcGivenTgt, which is the same except with source and target
 * completely reversed.
 *
 * Other variations that haven't been implemented are:
 * - Reverse conditional direction in probabilities
 * - Take maximum over all functions instead of (1-1) ones
 *
 * The idea is to have a sort of measure of how many words are well-translated.
 * \f$ \frac{P(t|s)}{max_{t' \in \mbox{tgt\_vocab}} P(t'|s)}\f$ is a sort of measure of whether t is a good
 * translation of s.
 * 
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group 
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada 
 */

#ifndef IBM1AARON_FF_H
#define IBM1AARON_FF_H

#include <featurefunction.h>
#include <ttablewithmax.h>
#include <vector>
#include <string>

using namespace Portage;

namespace Portage
{
  /// Interface for IBM1 Aaron.  Now called IBM1WTrans, but the base class
  /// still has its original name.
  class IBM1AaronBase: public FeatureFunction {
    private:
      TTableWithMax table;
    protected:
      /**
       * 
       * @param src  source tokens
       * @param tgt  target tokens
       * @return Returns
       */
      double computeValue(const Tokens& src, const Tokens& tgt);
    
    public:
      /// Constructor.
      /// @param file  file and arguments
      IBM1AaronBase(const string &file);
    
      virtual inline Uint requires() { return  FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
  };

  /// Forward IBM1Aaron.
  class IBM1WTransTgtGivenSrc: public IBM1AaronBase {
    public:
      /// Constructor.
      /// @param file  file and arguments
      IBM1WTransTgtGivenSrc(const string &file) : IBM1AaronBase(file) {}

      virtual inline double value(Uint k) {
        return computeValue((*src_sents)[s].getTokens(), (*nbest)[k].getTokens());
      }
  }; // IBM1AaronTgtGivenSrc
    
  /// Backward IBM1Aaron.
  class IBM1WTransSrcGivenTgt: public IBM1AaronBase {
    public:
      /// Constructor.
      /// @param file  file and arguments
      IBM1WTransSrcGivenTgt(const string &file) : IBM1AaronBase(file) {}
	    
      virtual inline double value(Uint k) {
        return computeValue((*nbest)[k].getTokens(), (*src_sents)[s].getTokens());
      }
  }; // IBM1AaronSrcGivenTgt
} // Portage

#endif // IBM1AARON_FF_H
