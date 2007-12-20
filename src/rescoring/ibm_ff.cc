/**
 * @author George Foster
 * @file ibm_ff.cc  Implementation of IBM1DocTgtGivenSrc
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

#include <file_utils.h>
#include "ibm_ff.h"

using namespace Portage;

IBM1DocTgtGivenSrc::IBM1DocTgtGivenSrc(const string& arg) 
: FeatureFunction(arg)
, ibm1(NULL)
, docids(NULL)
{}

bool IBM1DocTgtGivenSrc::parseAndCheckArgs()
{
   bool answer = true;
   if (split(argument, do_args, "#") != 2) {
      error(ETWarn, "bad argument to IBM1DocTgtGivenSrc: should be in format ibmfile");
      return false;
   }
   if (!check_if_exists(do_args[0])){
      error(ETWarn, "File is not accessible: %s", do_args[0].c_str());
      answer = false;
   }
   if (!check_if_exists(do_args[1])){
      error(ETWarn, "File is not accessible: %s", do_args[1].c_str());
      answer = false;
   }
   return answer;
}

bool IBM1DocTgtGivenSrc::loadModelsImpl()
{
   ibm1   = new IBM1(do_args[0]);
   docids = new DocID(do_args[1]);
   return ibm1 != NULL && docids != NULL;
}

void IBM1DocTgtGivenSrc::source(Uint s, const Nbest * const nbest)
{
   assert(docids);
   FeatureFunction::source(s, nbest);

   if (docids->isNewDoc(s)) {
      src_doc.clear();
      for (Uint i = s; i < s+docids->docSize(docids->docID(s)); ++i) {
         const string& ss((*src_sents)[i]);
	split(ss, src_doc);
      }
   }
}
