/**
 * @author George Foster
 * @file quick_set.cc Sets of integers with constant-time insert (amortized), find, and clear operations.
 *
 *
 * COMMENTS:
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */
#include <iostream>
#include "quick_set.h"
#include "errors.h"

using namespace Portage;

bool QuickSet::test() 
{
   Uint elems[] = {22,40,64,1,0,29,45,23,37,12};
   Uint offset_counts[ARRAY_SIZE(elems)];

   bool ok = true;
   QuickSet qs;

   for (Uint iter = 1; iter <= 2; ++iter) {

      for (Uint i = 0; i < ARRAY_SIZE(elems); ++i) {
	 if (qs.find(elems[i]) != qs.size()) {
	    error(ETWarn, "elem shouldn't be already in qs: %d", elems[i]);
	    ok = false;
	 }
	 qs.insert(elems[i]);
	 if (qs.find(elems[i]) == qs.size()) {
	    error(ETWarn, "elem should be already in qs: %d", elems[i]);
	    ok = false;
	 }
      }

      for (Uint i = 0; i < ARRAY_SIZE(elems); ++i)
	 offset_counts[i] = 0;
   
      for (Uint i = 0; i < ARRAY_SIZE(elems); ++i) {
	 if (qs.find(elems[i]) == qs.size()) {
	    error(ETWarn, "elem should be already in qs: %d", elems[i]);
	    ok = false;
	 }
	 ++offset_counts[qs.find(elems[i])];
      }

      for (Uint i = 0; i < ARRAY_SIZE(elems); ++i)
	 if (offset_counts[i] != 1) {
	    error(ETWarn, "offset count should be exactly 1");
	    ok = false;
	 }
   
      qs.clear();
   }
   return ok;
}

