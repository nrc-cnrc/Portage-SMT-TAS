/**
 * @author Evan Stratford
 * @file test_timer.h Tests the Timer class.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2007, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2007, Her Majesty in Right of Canada
 */

#ifndef TEST_TIMER_H
#define TEST_TIMER_H

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "timer.h"

using namespace Portage;

namespace Portage {

class MockTimer : public Timer {
   clock_t curr;
   virtual clock_t getTime() const { return curr; }
public:
   MockTimer() : Timer(), curr(0) {}
   void setTime(clock_t t) { curr = t; }
}; // class MockTimer

class TestTimer : public CxxTest::TestSuite {
public:
   void testTimer() {
      MockTimer t;
      t.setTime(50);
      t.reset();
      TS_ASSERT_EQUALS(t.ticksElapsed(), 0);
      t.setTime(1000);
      TS_ASSERT_EQUALS(t.ticksElapsed(), 950);
   }
}; // class TestTimer

} // namespace Portage

#endif
