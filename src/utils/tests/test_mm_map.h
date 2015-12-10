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
#include <fstream>

using namespace Portage;

namespace Portage {

class TestMemoryMappedMap : public CxxTest::TestSuite 
{
private:
   const string memory_map_filename;
public:
   TestMemoryMappedMap()
   : memory_map_filename("tests/test_mm_map.mmMap")
   {}

   void testCreatingMemoryMappedMap() {
      ostringstream input;
      input << "DDD" << ' ' << 'd' << endl;
      input << "A" << '\t' << '1' << endl;
      input << "E" << ' ' << '1' << endl;
      input << "C" << '\t' << "cc"<< endl;
      input << "BB" << ' ' << '1' << endl;

      istringstream is(input.str());
      ostringstream os;
      mkMemoryMappedMap(is, os);
      ofstream out(memory_map_filename.c_str());
      out << os.str();
   }

   void testValueIterator() {
      MMMap tim(memory_map_filename);

      TS_ASSERT(!tim.empty());

      TS_ASSERT_EQUALS(tim.size(), 5u);

      typedef MMMap::const_value_iterator IT;
      /*
      for (IT it(tim.vbegin()); it!=tim.vend(); ++it)
         cerr << *it << endl;
      */
      IT it = tim.vbegin();
      TS_ASSERT_EQUALS(string(*it), string("1"));

      ++it;
      TS_ASSERT_EQUALS(string(*it), string("cc"));

      ++it;
      TS_ASSERT_EQUALS(string(*it), string("d"));

      ++it;
      TS_ASSERT(it == tim.vend());
   }

   void testMMmapFind() {
      MMMap tim(memory_map_filename);

      TS_ASSERT(!tim.empty());

      TS_ASSERT_EQUALS(tim.size(), 5u);

      {
         MMMap::const_iterator it = tim.find("BB");
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "BB", 2);
         TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);
      }

      {
         MMMap::const_iterator it = tim.find(string("C"));
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "C", 1);
         TS_ASSERT_SAME_DATA(it.getValue(), "cc", 2);
      }

      {
         MMMap::const_iterator it = tim.find("A");
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "A", 1);
         TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);
      }

      {
         MMMap::const_iterator it = tim.find("DDD");
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "DDD", 3);
         TS_ASSERT_SAME_DATA(it.getValue(), "d", 1);
      }

      {
         MMMap::const_iterator it = tim.find("E");
         TS_ASSERT(it != tim.end());
         TS_ASSERT_SAME_DATA(it.getKey(), "E", 1);
         TS_ASSERT_SAME_DATA(it.getValue(), "1", 1);
      }

      {
         MMMap::const_iterator it = tim.find("D");
         TS_ASSERT(it == tim.end());
      }
   }

   void testTraversal() {
      MMMap tim(memory_map_filename);

      TS_ASSERT(!tim.empty());

      TS_ASSERT_EQUALS(tim.size(), 5u);

      typedef MMMap::const_iterator IT;

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
      MMMap tim(memory_map_filename);

      TS_ASSERT(!tim.empty());

      TS_ASSERT_EQUALS(tim.size(), 5u);

      TS_ASSERT_SAME_DATA(tim["A"], "1", 1);
      TS_ASSERT_SAME_DATA(tim[string("A")], "1", 1);
      TS_ASSERT_SAME_DATA(tim["z"], NULL, 1);
   }

}; // TestMemoryMappedMap

} // Portage
