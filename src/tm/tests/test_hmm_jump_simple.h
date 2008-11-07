/**
 * @author Eric Joanis
 * @file test_jmm_jump_simple.h  Test suite for HmmJumpSimple.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "hmm_jump_simple.h"

using namespace Portage;

namespace Portage {

class TestHMMJumpSimple : public CxxTest::TestSuite 
{
public:
   void testJumpPNoMaxJump() {
      HMMJumpSimple* js(NULL);
      TS_ASSERT_THROWS_NOTHING(
         js = dynamic_cast<HMMJumpSimple*>(
            HMMJumpStrategy::CreateNew(.2, 0.0, 0.1, 0.01, true, false, 0,
                                       NULL, 0, 0)));
      TS_ASSERT(js != NULL);
      js->backward_jump_p.push_back(20); // js->v_jump_p.setAt(-1) = 20;
      js->backward_jump_p.push_back(5);  // js->v_jump_p.setAt(-2) = 5;
      js->forward_jump_p.push_back(40);  // js->v_jump_p.setAt( 0) = 40;
      js->forward_jump_p.push_back(25);  // js->v_jump_p.setAt( 1) = 25;
      js->forward_jump_p.push_back(10);  // js->v_jump_p.setAt( 2) = 10;
      TS_ASSERT_DELTA(js->jump_p(3,5,10), 10, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(1,5,10), 0, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(6,3,10), 0, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(6,4,10), 5, 0.00001);
   }
   void testJumpPMaxJump() {
      HMMJumpSimple* js(NULL);
      TS_ASSERT_THROWS_NOTHING(
         js = dynamic_cast<HMMJumpSimple*>(
            HMMJumpStrategy::CreateNew(.2, 0.0, 0.1, 0.01, true, false, 2,
                                       NULL, 0, 0)));
      TS_ASSERT(js != NULL);
      js->backward_jump_p.push_back(20); // js->v_jump_p.setAt(-1) = 20;
      js->backward_jump_p.push_back(4);  // js->v_jump_p.setAt(-2) = 4;
      js->forward_jump_p.push_back(40);  // js->v_jump_p.setAt( 0) = 40;
      js->forward_jump_p.push_back(25);  // js->v_jump_p.setAt( 1) = 25;
      js->forward_jump_p.push_back(10);  // js->v_jump_p.setAt( 2) = 10;
      TS_ASSERT_DELTA(js->jump_p(3,4,9), 25, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(3,5,9), 2, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(1,5,12), 1, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(4,6,6), 10, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(4,6,7), 5, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(6,5,10), 20, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(6,4,10), 1, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(6,3,10), 1, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(3,1,10), 4, 0.00001);
      TS_ASSERT_DELTA(js->jump_p(4,2,10), 2, 0.00001);
   }
};

} // Portage
