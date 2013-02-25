/**
 * @author Eric Joanis
 * @file test_ordered_map.h  Test suite for OrderedMap
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "ordered_map.h"

using namespace Portage;

namespace Portage {

class TestOrderedMap : public CxxTest::TestSuite 
{
public:
   void setUp() {
   }
   void tearDown() {
   }
   void testEmpty() {
      OrderedMap<Uint,float> map;
      TS_ASSERT(map.empty());
   }
   void testInsert() {
      OrderedMap<Uint,float> map;
      TS_ASSERT_EQUALS(map.size(), 0u);
      map.set(9,9.9);
      TS_ASSERT_EQUALS(map.size(), 1u);
      map.set(4,4.4);
      TS_ASSERT_EQUALS(map.size(), 2u);
      TS_ASSERT(!map.empty());
   }
   void testInsertDuplicates() {
      OrderedMap<Uint,float> map;
      map.set(4,4.5);
      map.set(9,9.5);
      map.set(4,5.5);
      map.set(12,2.5);
      map.set(9,1.5);
      TS_ASSERT_EQUALS(map.size(), 3u);
      TS_ASSERT_EQUALS(map[4], 5.5);
      TS_ASSERT_EQUALS(map[9], 1.5);
      TS_ASSERT_EQUALS(map[12], 2.5);
      TS_ASSERT_EQUALS(map[3], 0.0);
   }
   void testDuplicatesNoSort() {
      OrderedMap<Uint,float> map;
      map.set(4,4.5);
      map.set(4,5.5);
      map.set(9,9.5);
      map.set(9,1.5);
      map.set(12,2.5);
      TS_ASSERT_EQUALS(map.size(), 3u);
      TS_ASSERT_EQUALS(map[4], 5.5);
      TS_ASSERT_EQUALS(map[9], 1.5);
      TS_ASSERT_EQUALS(map[12], 2.5);
      TS_ASSERT_EQUALS(map[3], 0.0);
   }
   void testNoDupsNoSort() {
      OrderedMap<Uint,float> map;
      map.set(4,4.5);
      map.set(9,9.5);
      map.set(12,2.5);
      TS_ASSERT_EQUALS(map.size(), 3u);
      TS_ASSERT_EQUALS(map[4], 4.5);
      TS_ASSERT_EQUALS(map[9], 9.5);
      TS_ASSERT_EQUALS(map[12], 2.5);
      TS_ASSERT_EQUALS(map[3], 0.0);
   }
   void testIteration() {
      OrderedMap<Uint,float> map;
      map.set(4,4.5);
      map.set(9,9.5);
      map.set(4,5.5);
      map.set(12,2.5);
      map.set(9,1.5);
      OrderedMap<Uint,float>::iterator it = map.begin(),
                                       end = map.end();
      TS_ASSERT(it != end);
      TS_ASSERT_EQUALS(it.index(), 4u);
      TS_ASSERT_EQUALS(it.key(), 4u);
      TS_ASSERT_EQUALS(*it, 5.5);
      ++it;
      TS_ASSERT(it != end);
      TS_ASSERT_EQUALS(it.index(), 9u);
      TS_ASSERT_EQUALS(it.key(), 9u);
      TS_ASSERT_EQUALS(*it, 1.5);
      ++it;
      TS_ASSERT(it != end);
      TS_ASSERT_EQUALS(it.index(), 12u);
      TS_ASSERT_EQUALS(it.key(), 12u);
      TS_ASSERT_EQUALS(*it, 2.5);
      ++it;
      TS_ASSERT(it == end);
   }
   void testIteration2() {
      OrderedMap<Uint,float> map;
      map.set(4,4.5);
      map.set(9,9.5);
      map.set(4,5.5);
      map.set(12,2.5);
      map.set(9,1.5);
      OrderedMap<Uint,float>::iterator it = map.begin(),
                                       end = map.end();
      ++it;
      TS_ASSERT(it != end);
      OrderedMap<Uint,float>::iterator it2 = it++;
      TS_ASSERT(it != end);
      TS_ASSERT_EQUALS(it2.index(), 9u);
      TS_ASSERT_EQUALS(it.index(), 12u);
      *it = 15;
      TS_ASSERT_EQUALS(map(12), 15);
   }
   void testConstIteration() {
      OrderedMap<Uint,float> map;
      map.set(4,4.5);
      map.set(9,9.5);
      map.set(4,5.5);
      map.set(12,2.5);
      map.set(9,1.5);
      const OrderedMap<Uint,float>& const_map(map);
      OrderedMap<Uint,float>::const_iterator it = const_map.begin(),
                                             end = const_map.end();
      TS_ASSERT(it != end);
      TS_ASSERT_EQUALS(it.index(), 4u);
      TS_ASSERT_EQUALS(it.key(), 4u);
      TS_ASSERT_EQUALS(*it, 5.5);
      ++it;
      TS_ASSERT(it != end);
      TS_ASSERT_EQUALS(it.index(), 9u);
      TS_ASSERT_EQUALS(it.key(), 9u);
      TS_ASSERT_EQUALS(*it, 1.5);
      ++it;
      TS_ASSERT(it != end);
      TS_ASSERT_EQUALS(it.index(), 12u);
      TS_ASSERT_EQUALS(it.key(), 12u);
      TS_ASSERT_EQUALS(*it, 2.5);
      ++it;
      TS_ASSERT(it == end);
   }
   void testConstIter2() {
      OrderedMap<pair<Uint,Uint>,float> map;
      map.set(4,5, 4);
      map.set(4,8, 8);
      map.set(6,0, 10);
      const OrderedMap<pair<Uint,Uint>,float>& const_map(map);
      OrderedMap<pair<Uint,Uint>,float>::const_iterator2<Uint>
         it(const_map.begin());
      TS_ASSERT_EQUALS(it.index(), 5u);
      ++it;
      TS_ASSERT_EQUALS(it.index(), 8u);
      ++it;
      TS_ASSERT_EQUALS(it.index(), 0u);
   }
   /*
   void notestMaxEnd() {
      OrderedMap<pair<Uint,Uint>,float> map;
      map.set(4,5, 4);
      map.set(4,8, 8);
      map.set(6,0, 10);
      const OrderedMap<pair<Uint,Uint>,float>& const_map(map);
      OrderedMap<pair<Uint,Uint>,float>::const_iterator
         it = const_map.begin(),
         end = const_map.end(make_pair(5,0));
      TS_ASSERT(it != end);
      ++it;
      TS_ASSERT(it != end);
      TS_ASSERT(end != it);
      ++it;
      TS_ASSERT(it == end);
      TS_ASSERT(end == it);
      TS_ASSERT(it != const_map.end());
      TS_ASSERT(const_map.end() != it);
      ++it;
      TS_ASSERT(it == end);
      TS_ASSERT(end == it);
      TS_ASSERT(it == const_map.end());
      TS_ASSERT(const_map.end() == it);
   }
   */
   void testLowerBound() {
      OrderedMap<pair<Uint,Uint>,float> map;
      map.set(4,5, 4);
      map.set(4,8, 8);
      map.set(6,0, 10);
      TS_ASSERT_EQUALS(map.lower_bound(4,0), map.begin());
      TS_ASSERT_EQUALS(map.lower_bound(5,0), ++ ++ map.begin());
      TS_ASSERT_EQUALS(map.lower_bound(6,0), ++ ++ map.begin());
      TS_ASSERT_EQUALS(map.lower_bound(7,0), map.end());
   }
   void testConstLowerBound() {
      OrderedMap<pair<Uint,Uint>,float> map;
      map.set(4,5, 4);
      map.set(4,8, 8);
      map.set(6,0, 10);
      const OrderedMap<pair<Uint,Uint>,float> const_map(map);
      TS_ASSERT_EQUALS(const_map.lower_bound(4,0), const_map.begin());
      TS_ASSERT_EQUALS(const_map.lower_bound(5,0), ++ ++ const_map.begin());
      TS_ASSERT_EQUALS(const_map.lower_bound(6,0), ++ ++ const_map.begin());
      TS_ASSERT_EQUALS(const_map.lower_bound(7,0), const_map.end());
   }
   void testSet() {
      OrderedMap<Uint,float> map;
      map.set(4, 4.5);
      map.set(4, 5.5);
      map.set(4, 6.5);
      TS_ASSERT_EQUALS(map.size(), 1u);
      TS_ASSERT_EQUALS(map[4], 6.5);
      map.set(6, 10);
      map.set(2, 1);
      TS_ASSERT_EQUALS(map.size(), 3u);
      TS_ASSERT_EQUALS(map[4], 6.5);
      TS_ASSERT_EQUALS(map[2], 1);
      TS_ASSERT_EQUALS(map[6], 10);
   }
   void testClear() {
      OrderedMap<Uint,float> map;
      map.set(4, 4.5);
      TS_ASSERT(!map.empty());
      map.clear();
      TS_ASSERT(map.empty());
   }
   void testOperator() {
      OrderedMap<Uint,float> map;
      map.set(4, 4.5);
      TS_ASSERT_EQUALS(map(4), 4.5);
   }
   void testPairOperator() {
      OrderedMap<pair<Uint,Uint>,float> map;
      map.set(4,5, 4.5);
      TS_ASSERT_EQUALS(map(4,5), 4.5);
      TS_ASSERT_EQUALS(map[make_pair(4,5)], 4.5);
      TS_ASSERT_EQUALS(map(5,4), 0);
      map.set<Uint,Uint>(6,7,6.5);
      map.set(6,7,6.5);
      //TS_ASSERT_EQUALS(map(6,7), 6.5);
      //TS_ASSERT_EQUALS(map[make_pair(6,7)], 6.5);
      //TS_ASSERT_EQUALS(map(7,6), 0);
   }
   void testCompact() {
      OrderedMap<Uint,float> map;
      map.set(4, 3);
      map.set(5, 2);
      map.set(4, 0);
      map.set(6, 7);
      TS_ASSERT_EQUALS(map.size(), 3u);
      map.compact();
      TS_ASSERT_EQUALS(map.size(), 2u);
      map.set(8, 0);
      map.compact();
      TS_ASSERT_EQUALS(map.size(), 2u);
   }
   void testAdditive() {
      OrderedMap<Uint,float> map;
      map.set(4,3);
      map.set(5,2);
      map.set(4,6);
      map.set(6,7);
      map.add(5,3);
      map.add(6,5);
      map.add(4,9);
      map.add(3,3);
      map.add(10,10);
      map.set(3,4);
      TS_ASSERT_EQUALS(map(3), 4);
      TS_ASSERT_EQUALS(map(4), 15);
      TS_ASSERT_EQUALS(map(5), 5);
      TS_ASSERT_EQUALS(map(6), 12);
      TS_ASSERT_EQUALS(map(10), 10);
   }
   void testLessThan() {
      OrderedMap<pair<Uint,Uint>,float> map;
      map.set(4,5, 4.5);
      map.set(6,7, 6.5);
      map.set(7,7, 6.5);
      TS_ASSERT(map.begin() < map.end());
      TS_ASSERT(++map.begin() < map.end());
      TS_ASSERT(++ ++map.begin() < map.end());
      TS_ASSERT(!(++ ++ ++map.begin() < map.end()));
      TS_ASSERT(! (map.begin() < map.lower_bound(0,0)));
   }
   void testFind() {
      OrderedMap<int,int> map;
      TS_ASSERT(map.find(2) == map.end());
      map.set(4,8);
      map.set(2,2);
      map.set(4,4);
      map.set(6,6);
      TS_ASSERT_EQUALS(map.find(2).key(), 2);
      TS_ASSERT_EQUALS(*map.find(4), 4);
      TS_ASSERT_EQUALS(map.find(6).key(), 6);
      TS_ASSERT(map.find(0) == map.end());
      TS_ASSERT(map.find(3) == map.end());
      TS_ASSERT(map.find(8) == map.end());
   }
   void testInsertFind() {
      OrderedMap<int,int> map;
      map.set(2,2);
      map.set(4,4);
      map.set(6,6);
      *map.insert_find(1) = 14;
      *map.insert_find(4) = 14;
      *map.insert_find(9) = 14;
      TS_ASSERT_EQUALS(map[0], 0);
      TS_ASSERT_EQUALS(map[1], 14);
      TS_ASSERT_EQUALS(map[2], 2);
      TS_ASSERT_EQUALS(map[3], 0);
      TS_ASSERT_EQUALS(map[4], 14);
      TS_ASSERT_EQUALS(map[6], 6);
      TS_ASSERT_EQUALS(map[9], 14);
   }
   void testEquality() {
      OrderedMap<int,int> map;
      map.set(2,2);
      map.set(6,6);
      map.set(4,4);
      OrderedMap<int,int> map2(map);
      TS_ASSERT(map == map2);
      map2.set(2,3);
      TS_ASSERT(map != map2);
      map2.set(2,2);
      TS_ASSERT(map == map2);
      map2.set(8,8);
      TS_ASSERT(map != map2);
      map.set(8,8);
      TS_ASSERT(map == map2);
      map.clear();
      TS_ASSERT(map != map2);
      map = map2;
      TS_ASSERT(map == map2);
      map2.clear();
      TS_ASSERT(map != map2);
      map2 = map;
      TS_ASSERT(map == map2);
      map2.set(3,3);
      TS_ASSERT(map != map2);
      map.set(5,5);
      TS_ASSERT(map != map2);
   }
}; // TestOrderedMap

} // Portage
