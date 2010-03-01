/**
 * @author Eric Joanis
 * @file test_str_utils.h  Test suite for the utilities in str_utils.{h,cc}
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "str_utils.h"

using namespace Portage;

namespace Portage {

class TestStrUtils : public CxxTest::TestSuite 
{
public:
   void testTrimChar() {
      char test_str[] = " \t asd qwe\t asdf \tasdf \t ";
      char test_str_answer[] = "asd qwe\t asdf \tasdf";
      TS_ASSERT(strcmp(trim(test_str), test_str_answer) == 0);
      char test_str2[] = "";
      TS_ASSERT(strcmp(trim(test_str2), "") == 0);
      char test_str3[] = "asdf";
      TS_ASSERT(strcmp(trim(test_str3), "asdf") == 0);
   }

   void testSplitString() {
      string s("a ||| b ||| c");
      vector<string> toks;
      string sep(" ||| ");
      
      splitString(s, toks, sep);
      TS_ASSERT_EQUALS(toks.size(), 3);
      TS_ASSERT_EQUALS(toks[0], "a");
      TS_ASSERT_EQUALS(toks[1], "b");
      TS_ASSERT_EQUALS(toks[2], "c");

      splitStringZ(s, toks, sep);
      TS_ASSERT_EQUALS(toks.size(), 3);
      TS_ASSERT_EQUALS(toks[0], "a");
      TS_ASSERT_EQUALS(toks[1], "b");
      TS_ASSERT_EQUALS(toks[2], "c");
   }

   void testConvUint() {
      Uint x = 42;
      TS_ASSERT(conv("0", x));
      TS_ASSERT_EQUALS(x, 0);
      TS_ASSERT(conv("1", x));
      TS_ASSERT_EQUALS(x, 1);
      TS_ASSERT(conv("2000000000", x));
      TS_ASSERT_EQUALS(x, 2000000000u);
      TS_ASSERT(conv("4000000000", x));
      TS_ASSERT_EQUALS(x, 4000000000u);
      TS_ASSERT(!conv("5000000000", x));
      TS_ASSERT(!conv("-1", x));
      TS_ASSERT(!conv("-2000000000", x));
      TS_ASSERT(!conv("-3000000000", x));
      TS_ASSERT(!conv("-5000000000", x));
   }

   void testConvInt() {
      int x = 42;
      TS_ASSERT(conv("0", x));
      TS_ASSERT_EQUALS(x, 0);
      TS_ASSERT(conv("1", x));
      TS_ASSERT_EQUALS(x, 1);
      TS_ASSERT(conv("2000000000", x));
      TS_ASSERT_EQUALS(x, 2000000000);
      TS_ASSERT(!conv("4000000000", x));
      TS_ASSERT(!conv("5000000000", x));
      TS_ASSERT(conv("-1", x));
      TS_ASSERT_EQUALS(x, -1);
      TS_ASSERT(conv("-2000000000", x));
      TS_ASSERT_EQUALS(x, -2000000000);
      TS_ASSERT(!conv("-3000000000", x));
   }

   void testConvUint64() {
      Uint64 x = 42;
      TS_ASSERT(conv("0", x));
      TS_ASSERT_EQUALS(x, 0);
      TS_ASSERT(conv("1", x));
      TS_ASSERT_EQUALS(x, 1);
      TS_ASSERT(conv("2000000000", x));
      TS_ASSERT_EQUALS(x, 2000000000u);
      TS_ASSERT(conv("4000000000", x));
      TS_ASSERT_EQUALS(x, 4000000000u);
      TS_ASSERT(conv("5000000000", x));
      TS_ASSERT_EQUALS(x, 5000000000ull);
      TS_ASSERT(conv("9223372036854775807", x)); // LLONG_MAX
      TS_ASSERT_EQUALS(x, 9223372036854775807ull);
      TS_ASSERT(conv("18446744073709551615", x)); // ULLONG_MAX
      TS_ASSERT_EQUALS(x, 18446744073709551615ull);
      TS_ASSERT(!conv("20000000000000000000", x));
      TS_ASSERT(!conv("-1", x));
      TS_ASSERT(!conv("-5000000000", x));
      TS_ASSERT(!conv("-9000000000000000000", x));
      TS_ASSERT(!conv("-15000000000000000000", x));
      TS_ASSERT(!conv("-20000000000000000000", x));
   }

   void testConvInt64() {
      Int64 x = 42;
      TS_ASSERT(conv("0", x));
      TS_ASSERT_EQUALS(x, 0);
      TS_ASSERT(conv("1", x));
      TS_ASSERT_EQUALS(x, 1);
      TS_ASSERT(conv("2000000000", x));
      TS_ASSERT_EQUALS(x, 2000000000);
      TS_ASSERT(conv("4000000000", x));
      TS_ASSERT_EQUALS(x, 4000000000ll);
      TS_ASSERT(conv("5000000000", x));
      TS_ASSERT_EQUALS(x, 5000000000ll);
      TS_ASSERT(conv("9223372036854775807", x)); // LLONG_MAX
      TS_ASSERT_EQUALS(x, 9223372036854775807ll);
      TS_ASSERT(!conv("18446744073709551615", x)); // ULLONG_MAX
      TS_ASSERT(!conv("20000000000000000000", x));
      TS_ASSERT(conv("-1", x));
      TS_ASSERT_EQUALS(x, -1);
      TS_ASSERT(conv("-5000000000", x));
      TS_ASSERT_EQUALS(x, -5000000000ll);
      TS_ASSERT(conv("-9000000000000000000", x));
      TS_ASSERT_EQUALS(x, -9000000000000000000ll);
      TS_ASSERT(!conv("-15000000000000000000", x));
      TS_ASSERT(!conv("-20000000000000000000", x));
   }

}; // TestStrUtils

} // Portage
