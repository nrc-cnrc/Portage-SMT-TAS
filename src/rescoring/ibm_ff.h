/**
 * @author George Foster
 * @file ibm_ff.h  Feature functions based on IBM models 1 and 2, in both
 * directions.
 * 
 * 
 * COMMENTS: 
 *
 * Feature functions based on IBM models 1 and 2, in both directions.
 *
 * Groupe de technologies langagieres interactives / Interactive Language Technologies Group
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#ifndef IBM_FF_H
#define IBM_FF_H

#include <numeric>
#include <ibm.h>
#include <str_utils.h>
#include "featurefunction.h"

using namespace Portage;

namespace Portage 
{

/// Forward IBM1
class IBM1TgtGivenSrc : public FeatureFunction
{
   IBM1 ibm1;

public:

   /**
    * Constructor, the model should have been trained for p(tgt|src).
    * @param ttable_file  file containing the ttable.
    */
   IBM1TgtGivenSrc(const string& ttable_file) : ibm1(ttable_file) {}

   virtual inline Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual double value(Uint k) {
      return ibm1.logpr((*src_sents)[s].getTokens(), (*nbest)[k].getTokens());
   }
};


/// Backward IBM1
class IBM1SrcGivenTgt : public FeatureFunction
{
   IBM1 ibm1;

public:

   /**
    * Constructor, the model should have been trained for p(src|tgt).
    * @param ttable_file  file containing the ttable.
    */
   IBM1SrcGivenTgt(const string& ttable_file) : ibm1(ttable_file) {}

   virtual inline Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual double value(Uint k) {
      return ibm1.logpr((*nbest)[k].getTokens(), (*src_sents)[s].getTokens());
   }
};


/// Forward IBM2
class IBM2TgtGivenSrc : public FeatureFunction
{
   IBM2 ibm2;

public:

   /**
    * Constructor, the model should have been trained for p(tgt|src).
    * @param ttable_file  file containing the ttable.
    */
   IBM2TgtGivenSrc(const string& ttable_file) : ibm2(ttable_file) {}

   virtual inline Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual double value(Uint k) {
      return ibm2.logpr((*src_sents)[s].getTokens(), (*nbest)[k].getTokens());
   }
};


/// Backward IBM2
class IBM2SrcGivenTgt : public FeatureFunction
{
   IBM2 ibm2;

public:

   /**
    * Constructor, the model should have been trained for p(src|tgt).
    * @param ttable_file  file containing the ttable.
    */
   IBM2SrcGivenTgt(const string& ttable_file) : ibm2(ttable_file) {}

   virtual inline Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual double value(Uint k) {
      return ibm2.logpr((*nbest)[k].getTokens(), (*src_sents)[s].getTokens());
   }
};

/// Calculate p(tgt-sent|src-doc) according to IBM1.
class IBM1DocTgtGivenSrc : public FeatureFunction
{
   /// Quick arguments processor.
   struct DoArgs {
      vector<string> args;  ///< arguments
      /// Extracts the arguments from arg.
      /// @param arg  arguments to parse.
      DoArgs(const string& arg) {
	 if (split(arg, args, ", ") != 2)
	    error(ETFatal, "bad argument to IBM1DocTgtGivenSrc: should be in format ibmfile,docfile");
      };
   } do_args;

   IBM1 ibm1;
   vector<Uint> doc_sizes;	///<ist of document sizes (# segs per doc)

   Uint curr_doc;
   Uint next_doc_start;

   vector<string> src_doc;

public:

   /**
    * Calculate p(tgt-sent|src-doc) according to IBM1.
    * @param arg format: "ibmfile,docfile", where ibmfile contains an ibm1
    * model and docfile specifies the sizes of consecutive documents within the
    * source file (separated by blanks).
    */
   IBM1DocTgtGivenSrc(const string& arg);

   virtual inline Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   /// See FeatureFunction::init(const Sentences * const src_sents, Uint K)
   virtual void init(const Sentences * const src_sents, Uint K) {
     FeatureFunction::init(src_sents, K);
     if ((*src_sents).size() != accumulate(doc_sizes.begin(), doc_sizes.end(), (Uint)0))
       error(ETFatal, "docfile contents don't match number of source sentences");
   }

   virtual void source(Uint s, const Nbest * const nbest);

   virtual double value(Uint k) {
      return ibm1.logpr(src_doc, (*nbest)[k].getTokens());
   }
};

}

#endif 
