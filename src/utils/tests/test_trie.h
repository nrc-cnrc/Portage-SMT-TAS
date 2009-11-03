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
}; // TestTrie

} // Portage
