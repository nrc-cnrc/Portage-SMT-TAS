/**
 * @author Eric Joanis
 * @file test_gfmath.h  Test suite for the utilities in gfmath.{h,cc}
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "gfmath.h"

using namespace Portage;

namespace Portage {

class TestGFMath : public CxxTest::TestSuite 
{
public:
   void testNextPowerOf2() {
      TS_ASSERT_EQUALS(next_power_of_2(0), 0u);
      TS_ASSERT_EQUALS(next_power_of_2(1), 1u);
      TS_ASSERT_EQUALS(next_power_of_2(2), 2u);
      for ( Uint exp = 2; exp < 32; ++exp ) {
         Uint pow = 1u << exp;
         TS_ASSERT_EQUALS(next_power_of_2(pow-1), pow);
         TS_ASSERT_EQUALS(next_power_of_2(pow  ), pow);
         TS_ASSERT_EQUALS(next_power_of_2(pow+1), 2*pow);
      }
   }
}; // TestGFMath

} // Portage
