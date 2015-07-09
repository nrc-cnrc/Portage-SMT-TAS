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

   void test_isSubset() {
      UintSet s1(1,Range(3,4));
      UintSet s2(1,Range(2,6));
      TS_ASSERT(isSubset(s1,s2));
      TS_ASSERT(!isSubset(s2,s1));

      s1.push_back(Range(5,6));
      TS_ASSERT(isSubset(s1,s2));
      TS_ASSERT(!isSubset(s2,s1));

      s1.push_back(Range(7,8));
      TS_ASSERT(!isSubset(s1,s2));
      TS_ASSERT(!isSubset(s2,s1));

      TS_ASSERT(isSubset(Range(2,6), s2));
      TS_ASSERT(!isSubset(Range(3,7), s2));
      TS_ASSERT(isSubset(s2, UintSet(1,Range(1,8))));
      TS_ASSERT(isSubset(s1, UintSet(1,Range(3,9))));
      TS_ASSERT(isSubset(UintSet(), UintSet(1,Range(2,3))));
      TS_ASSERT(isSubset(UintSet(), s1));
      TS_ASSERT(!isSubset(s1, UintSet()));

      TS_ASSERT(isSubset(Range(3,5), UintSet(1,Range(2,6))));
      TS_ASSERT(!isSubset(Range(3,5), UintSet(1,Range(4,6))));
      TS_ASSERT(!isSubset(Range(4,6), UintSet(1,Range(3,5))));
      TS_ASSERT(isSubset(Range(4,6), UintSet(1,Range(3,6))));
      TS_ASSERT(!isSubset(Range(3,6), UintSet(1,Range(4,6))));
      TS_ASSERT(!isSubset(Range(3,6), UintSet(1,Range(3,5))));
      TS_ASSERT(isSubset(Range(3,5), UintSet(1,Range(3,6))));

      UintSet s3(1,Range(1,5));
      s3.push_back(Range(7,9));
      s3.push_back(Range(12,15));
      TS_ASSERT(isSubset(Range(2,3),s3));
      TS_ASSERT(isSubset(Range(7,8),s3));
      TS_ASSERT(isSubset(Range(12,13),s3));
      TS_ASSERT(isSubset(Range(13,14),s3));
      TS_ASSERT(isSubset(Range(14,15),s3));
      TS_ASSERT(isSubset(Range(12,15),s3));
      TS_ASSERT(!isSubset(Range(0,1),s3));
      TS_ASSERT(!isSubset(Range(4,6),s3));
      TS_ASSERT(!isSubset(Range(6,7),s3));
      TS_ASSERT(!isSubset(Range(16,17),s3));
   }

   void test_isSubset_Range() {
      UintSet s2(1,Range(2,6));
      TS_ASSERT(isSubset(Range(3,4),s2));
      TS_ASSERT(!isSubset(Range(0,1),s2));
      TS_ASSERT(!isSubset(Range(1,3),s2));
      TS_ASSERT(!isSubset(Range(5,7),s2));
      TS_ASSERT(!isSubset(Range(7,9),s2));

      s2.push_back(Range(7,9));
      TS_ASSERT(isSubset(Range(7,8),s2));
      TS_ASSERT(isSubset(Range(4,6),s2));
      TS_ASSERT(!isSubset(Range(5,8),s2));
      TS_ASSERT(!isSubset(Range(11,12),s2));
      TS_ASSERT(!isSubset(Range(0,1),s2));
      TS_ASSERT(!isSubset(Range(6,7),s2));
   }

   void test_isDisjoint() {
      UintSet s(1,Range(2,4));
      s.push_back(Range(7,9));
      TS_ASSERT(isDisjoint(Range(0,1),s));
      TS_ASSERT(isDisjoint(Range(0,2),s));
      TS_ASSERT(isDisjoint(Range(4,5),s));
      TS_ASSERT(isDisjoint(Range(4,7),s));
      TS_ASSERT(isDisjoint(Range(6,7),s));
      TS_ASSERT(isDisjoint(Range(9,10),s));
      TS_ASSERT(isDisjoint(Range(10,11),s));
      TS_ASSERT(!isDisjoint(Range(1,3),s));
      TS_ASSERT(!isDisjoint(Range(2,3),s));
      TS_ASSERT(!isDisjoint(Range(3,4),s));
      TS_ASSERT(!isDisjoint(Range(3,5),s));
      TS_ASSERT(!isDisjoint(Range(3,8),s));
      TS_ASSERT(!isDisjoint(Range(6,8),s));
      TS_ASSERT(!isDisjoint(Range(7,8),s));
      TS_ASSERT(!isDisjoint(Range(6,10),s));
      TS_ASSERT(!isDisjoint(Range(8,10),s));
      TS_ASSERT(!isDisjoint(Range(1,10),s));
   }

   void test_bitfield() {
      class ArrayUint4 {
         Uint _v;
       public:
         ArrayUint4(Uint init = 0) : _v(init) {}
         Uint get(Uint i) { return (_v >> (4*i)) & 15; }
         void set(Uint i, Uint v) {
            assert(i <= 7);
            _v &= ~(15 << (4*i));
            _v |= (v&15) << (4*i);
         }
      };
      TS_ASSERT_EQUALS(sizeof(ArrayUint4), 4);
      struct MyTest {
         ArrayUint4 a;
         Uint b;
      };
      TS_ASSERT_EQUALS(sizeof(MyTest), 8);
      ArrayUint4 a(-1);
      TS_ASSERT_EQUALS(a.get(0), 15); TS_ASSERT_EQUALS(a.get(1), 15); TS_ASSERT_EQUALS(a.get(2), 15); TS_ASSERT_EQUALS(a.get(3), 15); TS_ASSERT_EQUALS(a.get(4), 15); TS_ASSERT_EQUALS(a.get(5), 15); TS_ASSERT_EQUALS(a.get(6), 15); TS_ASSERT_EQUALS(a.get(7), 15); 
      a.set(0, 3);
      TS_ASSERT_EQUALS(a.get(0), 3); TS_ASSERT_EQUALS(a.get(1), 15); TS_ASSERT_EQUALS(a.get(2), 15); TS_ASSERT_EQUALS(a.get(3), 15); TS_ASSERT_EQUALS(a.get(4), 15); TS_ASSERT_EQUALS(a.get(5), 15); TS_ASSERT_EQUALS(a.get(6), 15); TS_ASSERT_EQUALS(a.get(7), 15); 
      a.set(7,0);
      a.set(6,3);
      a.set(5,-2);
      TS_ASSERT_EQUALS(a.get(0), 3); TS_ASSERT_EQUALS(a.get(1), 15); TS_ASSERT_EQUALS(a.get(2), 15); TS_ASSERT_EQUALS(a.get(3), 15); TS_ASSERT_EQUALS(a.get(4), 15); TS_ASSERT_EQUALS(a.get(5), 14); TS_ASSERT_EQUALS(a.get(6), 3); TS_ASSERT_EQUALS(a.get(7), 0); 
      a.set(6,28);
      TS_ASSERT_EQUALS(a.get(0), 3); TS_ASSERT_EQUALS(a.get(1), 15); TS_ASSERT_EQUALS(a.get(2), 15); TS_ASSERT_EQUALS(a.get(3), 15); TS_ASSERT_EQUALS(a.get(4), 15); TS_ASSERT_EQUALS(a.get(5), 14); TS_ASSERT_EQUALS(a.get(6), 12); TS_ASSERT_EQUALS(a.get(7), 0); 
   }
}; // class TestCanoeGeneral

} // namespace Portage
