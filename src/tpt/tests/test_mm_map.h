/**
 * @author Samuel Larkin
 * @file tpt_tokenindex_map.h  Test suite for MMMap.
 *
 * Traitement multilingue de textes / Multilingual Text Processing
 * Tech. de l'information et des communications / Information and Communications Tech.
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2015, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2015, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "mm_map.h"

using namespace Portage;

namespace Portage {

class TestMemoryMappedMap : public CxxTest::TestSuite 
{
private:
   const string tokenIndexMapFilename;
public:
   TestMemoryMappedMap()
   : tokenIndexMapFilename("tests/test_mm_map.mmMap")
   {}

   void testCreatingTpClass() {
      ostringstream input;
      input << "DDD" << ' ' << 'd' << endl;
      input << "A" << '\t' << '1' << endl;
      input << "E" << ' ' << '1' << endl;
      input << "C" << '\t' << "cc"<< endl;
      input << "BB" << ' ' << '1' << endl;

      istringstream is(input.str());
      ostringstream os;
      ugdiss::mkMemoryMappedMap(is, os);
      ofstream out(tokenIndexMapFilename.c_str());
      out << os.str();
   }

   void testTokenIndexMap() {
      ugdiss::MMMap tim(tokenIndexMapFilename);

      TS_ASSERT(!tim.empty());

      TS_ASSERT_EQUALS(tim.size(), 5u);

      {
         ugdiss::MMMap::const_iterator it = tim.find("BB");
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "BB", 2);
         TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);
      }

      {
         ugdiss::MMMap::const_iterator it = tim.find(string("C"));
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "C", 1);
         TS_ASSERT_SAME_DATA(it.getValue(), "cc", 2);
      }

      {
         ugdiss::MMMap::const_iterator it = tim.find("A");
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "A", 1);
         TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);
      }

      {
         ugdiss::MMMap::const_iterator it = tim.find("DDD");
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "DDD", 3);
         TS_ASSERT_SAME_DATA(it.getValue(), "d", 1);
      }

      {
         ugdiss::MMMap::const_iterator it = tim.find("E");
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "E", 1);
         TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);
      }

      {
         ugdiss::MMMap::const_iterator it = tim.find("D");
         TS_ASSERT(it == tim.end());
      }
   }

   void testTraversal() {
      ugdiss::MMMap tim(tokenIndexMapFilename);

      TS_ASSERT(!tim.empty());

      TS_ASSERT_EQUALS(tim.size(), 5u);

      typedef ugdiss::MMMap::const_iterator IT;

      IT it(tim.begin());
      TS_ASSERT(it != tim.end());
      TS_ASSERT_SAME_DATA(it.getKey(), "A", 1);
      TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);

      ++it;
      TS_ASSERT(it != tim.end());
      TS_ASSERT_SAME_DATA(it.getKey(), "BB", 2);
      TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);

      it++;
      TS_ASSERT(it != tim.end());
      TS_ASSERT_SAME_DATA(it.getKey(), "C", 1);
      TS_ASSERT_SAME_DATA(it.getValue(), "cc", 2);

      ++it;
      TS_ASSERT(it != tim.end());
      TS_ASSERT_SAME_DATA(it.getKey(), "DDD", 3);
      TS_ASSERT_SAME_DATA(it.getValue(), "d", 1);

      ++it;
      TS_ASSERT(it != tim.end());
      TS_ASSERT_SAME_DATA(it.getKey(), "E", 1);
      TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);

      ++it;
      TS_ASSERT(it == tim.end());

      /*
      for (IT it(tim.begin()); it!=tim.end(); ++it)
         cerr << it.getKey() << '\t' << it.getValue() << endl;
      */
   }

   void testAccessor() {
      ugdiss::MMMap tim(tokenIndexMapFilename);

      TS_ASSERT(!tim.empty());

      TS_ASSERT_EQUALS(tim.size(), 5u);

      TS_ASSERT_SAME_DATA(tim["A"], "1", 1);
      TS_ASSERT_SAME_DATA(tim[string("A")], "1", 1);
      TS_ASSERT_SAME_DATA(tim["z"], NULL, 1);
   }

}; // TestMemoryMappedMap

} // Portage
