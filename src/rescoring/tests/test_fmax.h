/**
 * @author Samuel Larkin
 * @file test_fmax.h  Test suite for fmax based on Aaron Tikuisis unit test.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "fmax.h"

using namespace Portage;

namespace Portage {

class TestFmax : public CxxTest::TestSuite 
{
   double* f[3];
public:
   TestFmax()
   {
      f[0] = new double[3];
      f[1] = new double[3];
      f[2] = new double[3];
      f[0][0] = 1;
      f[0][1] = 4;
      f[0][2] = 3.1;
      f[1][0] = 2;
      f[1][1] = 6.1;
      f[1][2] = 1;
      f[2][0] = 3.1;
      f[2][1] = 1;
      f[2][2] = 2;
   }

   void test_fmax() {
      int func[3];
      max_1to1_func(f, (Uint)3, (Uint)3, func);

      TS_ASSERT_EQUALS(func[0], 2);
      TS_ASSERT_EQUALS(func[1], 1);
      TS_ASSERT_EQUALS(func[2], 0);
   }
}; // TestFmax

} // Portage
