/**
 * @author Eric Joanis
 * @file test_canoe_general.h  Unit test suite for functions in canoe_general.h
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "canoe_general.h"
#include <sstream>

using namespace Portage;

namespace Portage {

class TestCanoeGeneral : public CxxTest::TestSuite 
{
public:

   void test_range_output() {
      ostringstream oss;
      Range r(3,5);
      oss << r.toString();
      TS_ASSERT_EQUALS(oss.str(), "[3,5)");

      oss.str("");
      oss << r;
      TS_ASSERT_EQUALS(oss.str(), "[3, 5)");
   }

   void test_uintset_output() {
      UintSet s;
      s.push_back(Range(3,5));
      s.push_back(Range(7,9));
      ostringstream oss;
      oss << s;
      TS_ASSERT_EQUALS(oss.str(), "[3, 5) [7, 9)");
      TS_ASSERT_EQUALS(displayUintSet(s), "---11--11");
   }

   void test_uintset_ordering() {
      UintSet s1(1, Range(3,4));
      UintSet s2(1, Range(3,5));
      TS_ASSERT(s1 < s2);
      TS_ASSERT(!(s2 < s1));
      TS_ASSERT(!(s1 == s2));

      UintSet s3;
      TS_ASSERT(s3 < s1);
      TS_ASSERT(!(s1 < s3));
      TS_ASSERT(!(s3 == s1));

      s1[0].end = 5;
      TS_ASSERT(!(s1 < s2));
      TS_ASSERT(!(s2 < s1));
      TS_ASSERT(s2 == s1);

      s2.push_back(Range(7,9));
      TS_ASSERT(s1 < s2);
      TS_ASSERT(!(s2 < s1));
      TS_ASSERT(!(s1 == s2));

      s1[0].end = 6;
      TS_ASSERT(s2 < s1);
      TS_ASSERT(!(s1 < s2));
      TS_ASSERT(!(s2 == s1));
   }
}; // class TestCanoeGeneral

} // namespace Portage
