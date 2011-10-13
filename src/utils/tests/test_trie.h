/**
 * @author Eric Joanis
 * @file test_trie.h  Test suite for PTrie
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "trie.h"

using namespace Portage;

namespace Portage {

class MyInt {
   private:
      friend class TestTrie;
      static int in_existence;
      static int construct_count;
      static int destruct_count;
      size_t val; ///< value given in constructor
      Uint id; ///< unique instance id
   public:
      MyInt() : val(-1), id(++construct_count) { ++in_existence; }
      MyInt(int x) : val(x), id(++construct_count) { ++in_existence; }
      MyInt(const MyInt& x) : val(x.val), id(++construct_count)
         { ++in_existence; }
      ~MyInt() { ++destruct_count; --in_existence; }
      MyInt& operator=(const MyInt& x) { val = x.val; return *this; }
      MyInt& operator=(int x) { val = x; return *this; }
      int operator()() { return val; }
};
int MyInt::in_existence = 0;
int MyInt::construct_count = 0;
int MyInt::destruct_count = 0;

class TestTrie : public CxxTest::TestSuite 
{
public:
   void testDestructors() {
      PTrie<MyInt, Empty, true> trie;
      TrieKeyT key[3];
      key[0] = 1;
      key[2] = 3;
      Uint second_key[] = { 5, 3, 9, 10, 20, 35, 15, 2, 11, 49, 1, 0,
         66, 33, 22, 122, 4, 8, 150 };
      for ( Uint i = 0; i < ARRAY_SIZE(second_key); ++i ) {
         key[1] = second_key[i];
         trie.insert(key, 2, MyInt(i));
         TS_ASSERT_EQUALS(MyInt::in_existence, int(i+i+2));
         trie.insert(key, 3, MyInt(i));
         TS_ASSERT_EQUALS(MyInt::in_existence, int(i+i+3));
      }
      trie.clear();
      TS_ASSERT_EQUALS(MyInt::in_existence, 0);
   }

   void test_operator() {
      PTrie<Uint> trie;
      vector<Uint> key(3,2);
      trie[key] = 6;
      TS_ASSERT_EQUALS(trie[key], 6u);
      Uint val(0);
      TS_ASSERT(trie.find(&key[0], 3, val));
      TS_ASSERT_EQUALS(val, 6);
      TS_ASSERT(!trie.find(&key[0], 2, val));

      key[2] = 3;
      trie[key] += 10;
      TS_ASSERT(trie.find(&key[0], 3, val));
      TS_ASSERT_EQUALS(val, 10);
      TS_ASSERT(!trie.find(&key[0], 2, val));

      key[2] = 2;
      TS_ASSERT(trie.find(&key[0], 3, val));
      TS_ASSERT_EQUALS(val, 6);

      key[2] = 1;
      TS_ASSERT(!trie.find(&key[0], 3, val));

      vector<Uint> key2(6,2);
      TS_ASSERT_EQUALS(trie.at(&key2[2], 3), 6u);
      TS_ASSERT_EQUALS(trie.at(&key2[1], 4), 0u);

      const Uint* p_val(NULL);
      const PTrie<Uint>& const_trie(trie);
      TS_ASSERT(const_trie.find(&key2[2], 3, p_val));
      TS_ASSERT(p_val);
      if ( p_val )
         TS_ASSERT_EQUALS(*p_val, 6u);
   }
}; // TestTrie

} // Portage
