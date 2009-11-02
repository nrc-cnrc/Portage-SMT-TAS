/**
 * @author Eric Joanis
 * @file test_movable_trait.h  Test the movable_trait template.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2009, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2009, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include <boost/static_assert.hpp>
#include "movable_trait.h"

using namespace Portage;

namespace Portage {

struct Safe {};
SAFE_MOVABLE(Safe);
struct NotSafe : public Safe {};

// The tests in this test suite can be checked at compile time using
// BOOST_STATIC_ASSERT and at run time using TS_ASSERT_EQUALS;
// we do both, for the sake of completeness.
class TestMovableTrait : public CxxTest::TestSuite 
{
public:
   void testPrimitives() {
      TS_ASSERT_EQUALS(movable_trait<Uint>::safe, true);
      BOOST_STATIC_ASSERT(movable_trait<Uint>::safe == true);
      TS_ASSERT_EQUALS(movable_trait<Uint*>::safe, false);
      BOOST_STATIC_ASSERT(movable_trait<Uint*>::safe == false);
   }
   void testPair() {
      typedef std::pair<Uint,Uint> MovablePair;
      typedef std::pair<Uint,Uint*> UnmovablePair;
      TS_ASSERT_EQUALS(movable_trait<MovablePair>::safe, true);
      BOOST_STATIC_ASSERT(movable_trait<MovablePair>::safe == true);
      TS_ASSERT_EQUALS(movable_trait<UnmovablePair>::safe, false);
      BOOST_STATIC_ASSERT(movable_trait<UnmovablePair>::safe == false);
   }
   void testNoInherit() {
      TS_ASSERT_EQUALS(movable_trait<Safe>::safe, true);
      BOOST_STATIC_ASSERT(movable_trait<Safe>::safe == true);
      TS_ASSERT_EQUALS(movable_trait<NotSafe>::safe, false);
      BOOST_STATIC_ASSERT(movable_trait<NotSafe>::safe == false);
   }
}; // TestMovableTrait

} // Portage
