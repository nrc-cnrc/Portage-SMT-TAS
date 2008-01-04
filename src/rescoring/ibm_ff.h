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
 * Technologies langagieres interactives / Interactive Language Technologies
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
#include "docid.h"

using namespace Portage;

namespace Portage 
{

/// Forward IBM1
class IBM1TgtGivenSrc : public FeatureFunction
{
   IBM1* ibm1;

protected:
   virtual bool loadModelsImpl() {
      ibm1 = new IBM1(argument);
      assert(ibm1);
      return ibm1 != NULL;
   }

public:

   /**
    * Constructor, the model should have been trained for p(tgt|src).
    * @param ttable_file  file containing the ttable.
    */
   IBM1TgtGivenSrc(const string& ttable_file) 
   : FeatureFunction(ttable_file)
   , ibm1(NULL)
   {}
   virtual ~IBM1TgtGivenSrc() {
      if (ibm1) delete ibm1, ibm1 = NULL;
   }

   virtual bool parseAndCheckArgs() {
      if (argument.empty()) {
         error(ETWarn, "You must provide an IBM1 forward probability file to IBM1TgtGivenSrc");
         return false;
      }
      if (!check_if_exists(argument)){
         error(ETWarn, "File is not accessible: %s", argument.c_str());
         return false;
      }
      return true;
   }
   virtual Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual double value(Uint k) {
      return ibm1->logpr((*src_sents)[s].getTokens(), (*nbest)[k].getTokens());
   }
   virtual void values(Uint k, vector<double>& vals) {
      const vector<string>& src = (*src_sents)[s].getTokens();
      const vector<string>& tgt = (*nbest)[k].getTokens();
      for (vector<string>::const_iterator itr=tgt.begin(); itr!=tgt.end(); ++itr)
         vals.push_back(ibm1->minlogpr(src,*itr));
   }
};


/// Backward IBM1
class IBM1SrcGivenTgt : public FeatureFunction
{
   IBM1* ibm1;

protected:
   virtual bool loadModelsImpl() {
      ibm1 = new IBM1(argument);
      assert(ibm1);
      return ibm1 != NULL;
   }

public:

   /**
    * Constructor, the model should have been trained for p(src|tgt).
    * @param ttable_file  file containing the ttable.
    */
   IBM1SrcGivenTgt(const string& ttable_file)
   : FeatureFunction(ttable_file)
   , ibm1(NULL)
   {}
   virtual ~IBM1SrcGivenTgt() {
      if (ibm1) delete ibm1, ibm1 = NULL;
   }

   virtual bool parseAndCheckArgs() {
      if (argument.empty()) {
         error(ETFatal, "You must provide an IBM1 backward probability file to IBM1SrcGivenTgt");
         return false;
      }
      if (!check_if_exists(argument)){
         error(ETWarn, "File is not accessible: %s", argument.c_str());
         return false;
      }
      return true;
   }
   virtual Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual double value(Uint k) {
      return ibm1->logpr((*nbest)[k].getTokens(), (*src_sents)[s].getTokens());
   }
   virtual void values(Uint k, vector<double>& vals) {
      const vector<string>& src = (*src_sents)[s].getTokens();
      const vector<string>& tgt = (*nbest)[k].getTokens();
      for (vector<string>::const_iterator itr=tgt.begin(); itr!=tgt.end(); ++itr)
         vals.push_back(ibm1->minlogpr(*itr,src));
   }
};


/// Forward IBM2
class IBM2TgtGivenSrc : public FeatureFunction
{
   IBM2* ibm2;

protected:
   virtual bool loadModelsImpl() {
      ibm2 = new IBM2(argument);
      assert(ibm2);
      return ibm2 != NULL;
   }

public:

   /**
    * Constructor, the model should have been trained for p(tgt|src).
    * @param ttable_file  file containing the ttable.
    */
   IBM2TgtGivenSrc(const string& ttable_file)
   : FeatureFunction(ttable_file)
   , ibm2(NULL)
   {}
   virtual ~IBM2TgtGivenSrc() {
      if (ibm2) delete ibm2, ibm2 = NULL;
   }

   virtual bool parseAndCheckArgs() {
      if (argument.empty()) {
         error(ETFatal, "You must provide an IBM2 forward probability file to IBM2TgtGivenSrc");
         return false;
      }
      if (!check_if_exists(argument)){
         error(ETWarn, "File is not accessible: %s", argument.c_str());
         return false;
      }
      return true;
   }
   virtual Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual double value(Uint k) {
      return ibm2->logpr((*src_sents)[s].getTokens(), (*nbest)[k].getTokens());
   }
   virtual void values(Uint k, vector<double>& vals) {
      const vector<string>& src = (*src_sents)[s].getTokens();
      const vector<string>& tgt = (*nbest)[k].getTokens();
      for (Uint i=0; i<tgt.size(); ++i)
         vals.push_back(ibm2->minlogpr(src,tgt,i));
   }
};


/// Backward IBM2
class IBM2SrcGivenTgt : public FeatureFunction
{
   IBM2* ibm2;

protected:
   virtual bool loadModelsImpl() {
      ibm2 = new IBM2(argument);
      assert(ibm2);
      return ibm2 != NULL;
   }

public:

   /**
    * Constructor, the model should have been trained for p(src|tgt).
    * @param ttable_file  file containing the ttable.
    */
   IBM2SrcGivenTgt(const string& ttable_file)
   : FeatureFunction(ttable_file)
   , ibm2(NULL)
   {}
   virtual ~IBM2SrcGivenTgt() {
      if (ibm2) delete ibm2, ibm2 = NULL;
   }

   virtual bool parseAndCheckArgs() {
      if (argument.empty()) {
         error(ETFatal, "You must provide an IBM2 backwark probability file to IBM2SrcGivenTgt");
         return false;
      }
      if (!check_if_exists(argument)){
         error(ETWarn, "File is not accessible: %s", argument.c_str());
         return false;
      }
      return true;
   }
   virtual Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }
   virtual double value(Uint k) {
      return ibm2->logpr((*nbest)[k].getTokens(), (*src_sents)[s].getTokens());
   }
   virtual void values(Uint k, vector<double>& vals) {
      const vector<string>& src = (*src_sents)[s].getTokens();
      const vector<string>& tgt = (*nbest)[k].getTokens();
      for (Uint i=0; i<tgt.size(); ++i)
         vals.push_back(ibm2->minlogpr(tgt,src,i,true));
   }
};

/// Calculate p(tgt-sent|src-doc) according to IBM1.
class IBM1DocTgtGivenSrc : public FeatureFunction
{
   IBM1* ibm1;
   DocID* docids;

   vector<string> src_doc;
   vector<string> do_args;

protected:
   virtual bool parseAndCheckArgs();
   virtual bool loadModelsImpl();

public:

   /**
    * Calculate p(tgt-sent|src-doc) according to IBM1.
    * @param arg format: "ibmfile#docid_file", where ibmfile contains an ibm1
    * model and docid_file specifies the consecutive documents within the
    * source file (see docid.h for format description).
    */
   IBM1DocTgtGivenSrc(const string& arg);
   virtual ~IBM1DocTgtGivenSrc() {
      if (ibm1) delete ibm1, ibm1 = NULL;
      if (docids) delete docids, docids = NULL;
   }

   virtual Uint requires() { return FF_NEEDS_SRC_TOKENS | FF_NEEDS_TGT_TOKENS; }

   virtual FF_COMPLEXITY cost() const {return HIGH;}

   virtual void init(const Sentences * const src_sents) {
      assert(docids);
      FeatureFunction::init(src_sents);
      assert(src_sents != NULL);
      if (src_sents->size() != docids->numSrcLines())
         error(ETFatal, "docids contents don't match number of source sentences");
   }

   virtual void source(Uint s, const Nbest * const nbest);

   virtual double value(Uint k) {
      return ibm1->logpr(src_doc, (*nbest)[k].getTokens());
   }
   
   virtual void values(Uint k, vector<double>& vals) {
      const vector<string>& tgt = (*nbest)[k].getTokens();
      for (vector<string>::const_iterator itr=tgt.begin(); itr!=tgt.end(); ++itr)
         vals.push_back(ibm1->minlogpr(src_doc,*itr));
   }
};

}

#endif 
