/**
 * @author Eric Joanis
 * @file test_hmm.h Some test cases for HMMs.  Not thorough, just written to
 *                  fix bugs that came up.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de tech. de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "hmm.h"

using namespace Portage;

namespace Portage {

class TestHMM : public CxxTest::TestSuite 
{
   enum CrazyStates { CP, IP };
   enum CrazyAlphabet { cola, ice_t, lem };
   static const Uint N = 2;
   static const Uint M = 3;
   static const Uint T = 3;
   uintVector O;
   HMM* crazy;
public:
   void setUp() {
      crazy = new HMM(N, M, HMM::state_exit_emit, HMM::regular_probs);
      crazy->A(CP,CP) = .7;
      crazy->A(CP,IP) = .3;
      crazy->A(IP,CP) = .5;
      crazy->A(IP,IP) = .5;

      crazy->B(CP,cola) = .6;
      crazy->B(CP,ice_t) = .1;
      crazy->B(CP,lem) = .3;

      crazy->B(IP,cola) = .1;
      crazy->B(IP,ice_t) = .7;
      crazy->B(IP,lem) = .2;

      O.resize(T);
      O[0] = lem;
      O[1] = ice_t;
      O[2] = cola;
   }
   void tearDown() {
      delete crazy;
      O.clear();
   }
   void testSetUp() {
      TS_ASSERT(crazy->checkTransitionDistributions());
      TS_ASSERT(crazy->checkEmissionDistributions(true));
   }
   void testZeroProb() {
      // We make one state be impossible to emit, thus any path through the HMM
      // should have 0.0 probability and P(O|mu) has to be 0.0.
      crazy->B(CP,ice_t) = 0.0;
      crazy->B(IP,ice_t) = 0.0;
      dMatrix alpha_hat;
      dVector c;
      // Note: can't use TS_ASSERT_EQUALS or TS_ASSERT_DELTA since they enter
      // an infinite loop when faced with -INFINITY.
      TS_ASSERT(crazy->ForwardProcedure(O,alpha_hat,c,false) == log(0.0));
      TS_ASSERT(crazy->ForwardProcedure(O,alpha_hat,c,true) == log(0.0));
   }
}; // TestHMM

} // Portage

