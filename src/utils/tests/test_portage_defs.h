/**
 * @author Eric Joanis
 * @file test_portage_defs.h  Test suite for stuff in portage_defs.h
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2012, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2012, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"

using namespace Portage;

namespace Portage {

class TestPortageDefs : public CxxTest::TestSuite 
{
public:
   void test_array_size() {
      const char * array0[] = { };
      TS_ASSERT_EQUALS(ARRAY_SIZE(array0), 0);
      const char * array1[] = { "a" };
      TS_ASSERT_EQUALS(ARRAY_SIZE(array1), 1);
      const char * array3[] = { "a", "b", "c" };
      TS_ASSERT_EQUALS(ARRAY_SIZE(array3), 3);
      const char * array10[] = { "a", "b", "c", "a", "b", "c", "a", "b", "c", "d" };
      TS_ASSERT_EQUALS(ARRAY_SIZE(array10), 10);
      Uint arrayuint[] = { 1, 2, 3, 4 };
      TS_ASSERT_EQUALS(ARRAY_SIZE(arrayuint), 4);
      /* This fails to compiles, as it should:
      vector<string> v(3, "asdf");
      TS_ASSERT_EQUALS(ARRAY_SIZE(v), 3);
      */
   }
   void test_and() {
      //TS_ASSERT_EQUALS(1 && 3, 3); // No!  1 && 3 is true, not 3 - this is C++, not bash
   }
   void test_min_max() {
      TS_ASSERT_EQUALS(max(-4,6), 6);
      TS_ASSERT_EQUALS(max(6,-4), 6);
      TS_ASSERT_EQUALS(min(-4,6), -4);
      TS_ASSERT_EQUALS(min(6,4), 4);
   }
}; // TestPortageDefs

} // Portage
