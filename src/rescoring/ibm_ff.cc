/**
 * @author George Foster
 * @file ibm_ff.cc  Implementation of IBM1DocTgtGivenSrc
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

#include <file_utils.h>
#include "ibm_ff.h"

using namespace Portage;

IBM1DocTgtGivenSrc::IBM1DocTgtGivenSrc(const string& arg) : 
   do_args(arg), ibm1(do_args.args[0])
{
   string fn;
   if (!split(gulpFile(do_args.args[1].c_str(), fn), doc_sizes))
      error(ETFatal, "can't convert docfile contents to sequence of integers");

   curr_doc = 0;
   next_doc_start = 0;
}

void IBM1DocTgtGivenSrc::source(Uint s, const Nbest * const nbest)
{
   if (s == next_doc_start) {

      src_doc.clear();
      for (Uint i = 0; i < doc_sizes[curr_doc]; ++i) {
	string ss((*src_sents)[i]);
	split(ss, src_doc);
      }

      next_doc_start += doc_sizes[curr_doc++];
   }
}
