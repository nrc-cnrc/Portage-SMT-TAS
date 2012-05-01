/**
 * @author Evan Stratford
 * @file test_stats.h Tests the Stat and StatCollection classes.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef TEST_STATS_H
#define TEST_STATS_H

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include <sstream>
#include "stats.h"

using namespace Portage;

namespace Portage {

class TestStats : public CxxTest::TestSuite {
public:
   void testStat() {
   }
   
   void testCountStat() {
      CountStat s("test-count-stat");
      TS_ASSERT_EQUALS(s.c, 0);
      s.incr();
      TS_ASSERT_EQUALS(s.c, 1);
      s.incr();
      TS_ASSERT_EQUALS(s.c, 2);
   }

   void testTotalStat() {
      TotalStat s("test-total-stat");
      TS_ASSERT_EQUALS(s.c, 0);
      s.add(4);
      TS_ASSERT_EQUALS(s.c, 4);
      s.add(-3);
      TS_ASSERT_EQUALS(s.c, 1);
      s.add();
      TS_ASSERT_EQUALS(s.c, 2);
   }

   void testAvgVarStat() {
      double epsilon = 1e-4;
      AvgVarStat s("test-avg-var-stat");
      s.add(4);
      s.add(7);
      s.add(13);
      s.add(16);
      TS_ASSERT_EQUALS(s.n, 4);
      TS_ASSERT_DELTA(s.m, 10.0, epsilon);
      TS_ASSERT_DELTA(s.v, 90.0, epsilon);
   }

   void testHistogramStat() {
      HistogramStat s("test-histogram-stat");
      s.add(2);
      s.add(3);
      s.add(4);
      s.add(2);
      s.add(2);
      s.add(3);
      s.add(1);
      s.add(4, 4);
      TS_ASSERT_EQUALS(s.h[1], 1);
      TS_ASSERT_EQUALS(s.h[2], 3);
      TS_ASSERT_EQUALS(s.h[3], 2);
      TS_ASSERT_EQUALS(s.h[4], 5);
   }
};

}

#endif
