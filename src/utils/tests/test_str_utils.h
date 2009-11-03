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
}; // TestStrUtils

} // Portage
