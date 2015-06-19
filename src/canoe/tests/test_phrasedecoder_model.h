/**
 * @author Eric Joanis
 * @file test_phrasedecoder_model.h  Unit test suite for functions in * phrasedecoder_model.h
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2011, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2011, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "phrasedecoder_model.h"
#include <sstream>

using namespace Portage;

namespace Portage {

class TestPhraseDecoderModel : public CxxTest::TestSuite 
{
public:

   void test_ArrayUint4() {
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

      ArrayUint4 b(3, 5);
      TS_ASSERT_EQUALS(b.get(0), 3); TS_ASSERT_EQUALS(b.get(1), 3); TS_ASSERT_EQUALS(b.get(2), 3); TS_ASSERT_EQUALS(b.get(3), 3); TS_ASSERT_EQUALS(b.get(4), 3); TS_ASSERT_EQUALS(b.get(5), 0); TS_ASSERT_EQUALS(b.get(6), 0); TS_ASSERT_EQUALS(b.get(7), 0); 
      b = ArrayUint4(5, 1);
      TS_ASSERT_EQUALS(b.get(0), 5); TS_ASSERT_EQUALS(b.get(1), 0); TS_ASSERT_EQUALS(b.get(2), 0); TS_ASSERT_EQUALS(b.get(3), 0); TS_ASSERT_EQUALS(b.get(4), 0); TS_ASSERT_EQUALS(b.get(5), 0); TS_ASSERT_EQUALS(b.get(6), 0); TS_ASSERT_EQUALS(b.get(7), 0); 
   }
}; // class TestCanoeGeneral

} // namespace Portage
