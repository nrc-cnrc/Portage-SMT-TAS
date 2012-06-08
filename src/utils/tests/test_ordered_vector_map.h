/**
 * @author Eric Joanis
 * @file test_ordered_vector_map.h  Partial test suite for ordered_vector_map
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2010, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2010, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "ordered_vector_map.h"

using namespace Portage;

namespace Portage {

class TestOrderedVectorMap : public CxxTest::TestSuite 
{
public:
   typedef ordered_vector_map<Uint,Uint,true> VMap;
   void test_ordering() {
      VMap v;
      TS_ASSERT(v.test_is_sorted());
      TS_ASSERT_EQUALS(v.size(), 0u);
      v[10] = 3;
      TS_ASSERT(v.test_is_sorted());
      TS_ASSERT_EQUALS(v.size(), 1u);
      v[2] = 3;
      TS_ASSERT(v.test_is_sorted());
      TS_ASSERT_EQUALS(v.size(), 2u);
      v[5] = 3;
      TS_ASSERT(v.test_is_sorted());
      TS_ASSERT_EQUALS(v.size(), 3u);
      v[20] = 3;
      TS_ASSERT(v.test_is_sorted());
      TS_ASSERT_EQUALS(v.size(), 4u);
      v[1] = 3;
      TS_ASSERT(v.test_is_sorted());
      TS_ASSERT_EQUALS(v.size(), 5u);
      v[5] += 10;
      TS_ASSERT(v.test_is_sorted());
      TS_ASSERT_EQUALS(v.size(), 5u);

      VMap::iterator it = v.find(5);
      TS_ASSERT(it != v.end());
      if ( it != v.end() ) {
         TS_ASSERT_EQUALS(it->second, 13u);
         TS_ASSERT_EQUALS(it->first, 5u);
      }
      TS_ASSERT(v.find(0) == v.end());
      TS_ASSERT(v.find(6) == v.end());
      TS_ASSERT(v.find(30) == v.end());

      const VMap& v2(v);
      VMap::const_iterator c_it = v2.find(2);
      TS_ASSERT(c_it != v2.end());
      if ( c_it != v2.end() ) {
         TS_ASSERT_EQUALS(c_it->first, 2u);
         TS_ASSERT_EQUALS(c_it->second, 3u);
      }
      TS_ASSERT(v2.find(0) == v2.end());
      TS_ASSERT(v2.find(7) == v2.end());
      TS_ASSERT(v2.find(40) == v2.end());
   }
      
   void test_union() {
      VMap v1, v2;
      v1[4] = 5;
      v1[2] = 3;
      v1[10] = 2;
      v2[5] = 2;
      v2[4] = 1;

      TS_ASSERT(v1.test_is_sorted());
      TS_ASSERT(v2.test_is_sorted());

      v1 += v2;
      TS_ASSERT_EQUALS(v1.size(), 4u);
      TS_ASSERT_EQUALS(v1[2], 3u);
      TS_ASSERT_EQUALS(v1[4], 6u);
      TS_ASSERT_EQUALS(v1[5], 2u);
      TS_ASSERT_EQUALS(v1[10], 2u);
      TS_ASSERT_EQUALS(v1[3], 0u);
      TS_ASSERT_EQUALS(v1.size(), 5u);

      TS_ASSERT(v1.test_is_sorted());
      TS_ASSERT(v2.test_is_sorted());
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

   typedef ordered_vector_map<int, vector<int>, true> VMap2;
   void test_subvector() {
      VMap2 v;
      TS_ASSERT_EQUALS(v.size(), 0u);
      v[5].assign(5, 25);
      TS_ASSERT_EQUALS(v.size(), 1u);
      v[3].assign(3, 9);
      TS_ASSERT_EQUALS(v.size(), 2u);
      v[10].assign(10, 100);
      TS_ASSERT_EQUALS(v.size(), 3u);
      TS_ASSERT_EQUALS(v[3].size(), 3u);
      TS_ASSERT_EQUALS(v[5].size(), 5u);
      TS_ASSERT_EQUALS(v[10].size(), 10u);
      TS_ASSERT_EQUALS(v[9].size(), 0u);
      TS_ASSERT_EQUALS(v[11].size(), 0u);
      TS_ASSERT_EQUALS(v[-1].size(), 0u);
      TS_ASSERT_EQUALS(v.size(), 6u);
      TS_ASSERT_EQUALS(v[3].size(), 3u);
      TS_ASSERT_EQUALS(v[5].size(), 5u);
      TS_ASSERT_EQUALS(v[10].size(), 10u);
   }

   void test_managed_resize() {
      VMap2 v;
      for (Uint i = 1; i < 136; ++i) {
         v[i].assign(i,i);
         TS_ASSERT_EQUALS(v.size(), i);
         //TS_ASSERT_EQUALS(v.capacity(), i);
         TS_ASSERT_EQUALS(v[i].size(), i);
      }
   }

   struct NotSafe1 {
      NotSafe1* self;
      NotSafe1() { self = this; }
   };
   struct NotSafe2 {
      Uint* p;
      NotSafe2() { p = new Uint; }
      NotSafe2(const NotSafe2&) { p = new Uint; }
      NotSafe2& operator=(const NotSafe2&) { p = new Uint; return *this; }
      ~NotSafe2() { delete p; }
   };
   void test_test_safe_memmove() {
      TS_ASSERT_EQUALS((ordered_vector_map<Uint,Uint>::test_safe_memmove()), true);
      TS_ASSERT_EQUALS((ordered_vector_map<Uint,vector<Uint> >::test_safe_memmove()), true);
      TS_ASSERT_EQUALS((ordered_vector_map<Uint,NotSafe1>::test_safe_memmove()), false);
      TS_ASSERT_EQUALS((ordered_vector_map<Uint,NotSafe2>::test_safe_memmove()), false);
      ordered_vector_map<Uint,Uint,true> w;
      ordered_vector_map<Uint,vector<Uint>,true> z;
      ordered_vector_map<Uint,NotSafe1,false> x;
      ordered_vector_map<Uint,NotSafe2,false> y;
   }

}; // TestOrderedVectorMap


} // namespace Portage
