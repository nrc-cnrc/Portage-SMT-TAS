/**
 * @author INSERT YOUR NAME HERE
 * @file test_gfstats.h  Test suite for gfstats
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "gfstats.h"

using namespace Portage;

namespace Portage {

class TestGFStats : public CxxTest::TestSuite 
{
public:

   vector<double> x;

   void setUp() {
      x.push_back(12.0);
      x.push_back(8.0);
      x.push_back(10.0);
   }
   void tearDown() {}

   void testMeanVar() {
      double m = mean(x.begin(), x.end());
      double v = var(x.begin(), x.end());
      pair<double,double> mv = meanvar(x.begin(), x.end());
      TS_ASSERT_EQUALS(m, 10.0);
      TS_ASSERT_EQUALS(v, 4.0);
      TS_ASSERT_EQUALS(m, mv.first);
      TS_ASSERT_EQUALS(v, mv.second);
   }

   void testMeanVarNorm() {
      vector<double> y = x;     // in case test order can change
      meanvarnorm(y.begin(), y.end());
      pair<double,double> mv = meanvar(y.begin(), y.end());
      TS_ASSERT_EQUALS(mv.first, 0.0);
      TS_ASSERT_EQUALS(sqrt(mv.second), 1.0);
   }

}; // TestGFStats

} // Portage
