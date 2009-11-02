/**
 * @author Samuel Larkin
 * @file test_simple_histogram.h Tests the simple histogram's functionalities
 *
 * $Id$
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef __TEST_CACHE_VALUE_H__
#define __TEST_CACHE_VALUE_H__

#include <cxxtest/TestSuite.h>
#include "cache_value.h"

using namespace std;
using namespace Portage;

namespace Portage {

/// Tests the implementation of SimpleHistogram
class test_cacheValue : public CxxTest::TestSuite 
{
   private:
      static double func_nlog2n(double n) {
         if(n <= 0) return 0;
         return n * (log(n)/log(2));
      }

   public:
      void test_nlogn() {
         cacheValue<double, double (*)(double)> nlogn(func_nlog2n, 10, 1);

         TS_ASSERT(nlogn.inCache(0) == false);
         TS_ASSERT(nlogn.inCache(1) == true);
         TS_ASSERT(nlogn.inCache(9) == true);
         TS_ASSERT(nlogn.inCache(10) == false);

         TS_ASSERT_DELTA(nlogn(4), 8.0, 0.0001);
         TS_ASSERT_DELTA(nlogn(16), 64.0, 0.0001);
      }
};

} // ends namespace Portage

#endif /// __TEST_CACHE_VALUE_H__
