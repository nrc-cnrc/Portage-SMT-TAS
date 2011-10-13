/**
 * @author Eric Joanis
 * @file test_vector_map.h  Partial test suite for vector_map
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "vector_map.h"

using namespace Portage;

namespace Portage {

class TestVectorMap : public CxxTest::TestSuite 
{
   typedef vector_map<Uint,Uint> VMap;
public:
   void test_union() {
      VMap v1, v2;
      v1[4] = 5;
      v1[2] = 3;
      v1[10] = 2;
      v2[5] = 2;
      v2[4] = 1;

      v1 += v2;
      TS_ASSERT_EQUALS(v1.size(), 4u);
      TS_ASSERT_EQUALS(v1[2], 3u);
      TS_ASSERT_EQUALS(v1[4], 6u);
      TS_ASSERT_EQUALS(v1[5], 2u);
      TS_ASSERT_EQUALS(v1[10], 2u);
      TS_ASSERT_EQUALS(v1[3], 0u);
      TS_ASSERT_EQUALS(v1.size(), 5u);
   }

   void test_max() {
      VMap v1;
      v1[4] = 5;
      v1[2] = 3;
      v1[10] = 2;
      TS_ASSERT_EQUALS(v1.max()->first, 4u);

      v1[2] = 10;
      TS_ASSERT_EQUALS(v1.max()->first, 2u);

      v1[3] = 13;
      TS_ASSERT_EQUALS(v1.max()->first, 3u);

      v1[3] = 3;
      TS_ASSERT_EQUALS(v1.max()->first, 2u);

      const VMap c_v(v1);
      TS_ASSERT_EQUALS(c_v.max()->first, 2u);

      VMap v2;
      TS_ASSERT(v2.max() == v2.end());
   }

}; // TestVectorMap


} // namespace Portage
