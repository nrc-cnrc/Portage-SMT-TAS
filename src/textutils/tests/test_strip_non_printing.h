/**
 * @author Samuel Larkin
 * @file test_strip_non_printing.h  Test suite for strip_non_printing
 *
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"

#include "MagicStream.h"
#include "arg_reader.h"

// Hack to hide the main function.
// Allows us to use other functions.
namespace tested {
#include "../strip_non_printing.cc"
}

using namespace Portage;

namespace Portage {

class TestStripNonPrinting : public CxxTest::TestSuite 
{
public:
   void testIsRegular() {
      // Specific test to ease our mind.
      TS_ASSERT(tested::isRegular('a'));
      TS_ASSERT(!tested::isRegular('\n'));
      TS_ASSERT(!tested::isRegular(''));

      // Lets to a thorough testing of characters.
      // These are control characters.
      for (char c=0; c<=0x20; ++c)
         TS_ASSERT(!tested::isRegular(c));

      // These are regular characters.
      for (char c=0xff; c>0x20; --c) {
         // Delete should be a control character.
         if (c == 0x7f) {
            TS_ASSERT(!tested::isRegular(c));
         }
         else {
            TS_ASSERT(tested::isRegular(c));
         }
      }
   }
   void testStripNonPrinting() {
      istringstream in("aaaa  aa\n\n	\n	 ddd\ne	 eeee\nf	   fff	f\nzzZz  zZ\ny yy\nThis is a simple test to see if words get splitted.\nIl faudrait l'apostrophe dans un test!\n");
      ostringstream out;
      tested::stripNonPrinting(in, out);
      const string ref("a aaa aa\n\n\nddd\ne eeee\nf fff f\nzzZz zZ\ny yy\nThis is a simple test to see if words get splitted.\nIl faudrait l'apostrophe dans un test!\n");
      TS_ASSERT_EQUALS(ref, out.str());
   }
}; // TestStripNonPrinting

} // Portage
