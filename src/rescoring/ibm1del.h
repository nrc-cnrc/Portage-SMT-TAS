/**
 * @author Nicola Ueffing
 * @file ibm1del.h  Contains the declaration of IBM1Deletion.
 *
 * $Id$
 * 
 * N-best Rescoring Module
 * 
 * Contains the declaration of IBM1Deletion, which is a feature function
 * defined as follows:
 * \f$ \frac{1}{J} \sum\limits_{j=1}^{J} \delta ( \max\limits_{e\in \mbox{tgt\_vocab}} p(e|f_j)) \f$
 *
 * The idea is to have a sort of measure of how many source words have not been translated (or been translated badly).
 * 
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology 
 * Conseil national de recherches Canada / National Research Council Canada 
 * Copyright 2006, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2006, Her Majesty in Right of Canada 
 */

#ifndef IBM1DEL_FF_H
#define IBM1DEL_FF_H

#include <featurefunction.h>
#include <ttablewithmax.h>
#include <vector>
#include <string>

using namespace Portage;

namespace Portage
{
  /// Interface for IBM1Deletion
  class IBM1DeletionBase : public FeatureFunction {
    private:
      TTableWithMax*  table;
      double        thr;   ///< threshold for distinguishing between deletions/good translations.
      string          ttable_file;

    protected:
      /**
       *
       * @param src  source tokens
       * @param tgt  target tokens
       * @return Returns Needs description
       */
      double computeValue(const Tokens& src, const Tokens& tgt);
      virtual bool parseAndCheckArgs();
      virtual bool loadModelsImpl();
    public:
      /// Constructor.
      /// @param args  arguments.
      IBM1DeletionBase(const string& args); 
      virtual ~IBM1DeletionBase() {
         if (table) delete table, table = NULL;
      }
   
      virtual inline Uint requires() { return  FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
  };

  /// Forward IBM1Deletion
  class IBM1DeletionTgtGivenSrc: public IBM1DeletionBase {
    public:
      /// Constructor.
      /// @param args  arguments.
      IBM1DeletionTgtGivenSrc(const string &args) : IBM1DeletionBase(args) {}
      
      virtual inline double value(Uint k) {
        return computeValue((*src_sents)[s].getTokens(), (*nbest)[k].getTokens());
      }
  }; // IBM1DeletionTgtGivenSrc
  
  /// Backward IBM1Deletion
  class IBM1DeletionSrcGivenTgt: public IBM1DeletionBase {
    public:
      /// Constructor.
      /// @param args  arguments.
      IBM1DeletionSrcGivenTgt(const string &args) : IBM1DeletionBase(args) {}
      
      virtual inline double value(Uint k) {
        return computeValue((*nbest)[k].getTokens(), (*src_sents)[s].getTokens());
      }
  }; // IBM1DeletionSrcGivenTgt
} // Portage

#endif // IBM1DELETION_FF_H
