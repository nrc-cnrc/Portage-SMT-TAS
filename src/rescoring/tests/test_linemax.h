/**
 * @author Samuel Larkin
 * @file test_linemax.h  Test suite for Linemax based on Aaron Tikuisis unit
 *                       test for linemax algorithm.
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "linemax.h"
#include "eval_weights.h"
#include "linearmetric.h"

using namespace Portage;

namespace Portage {

class TestLinemax : public CxxTest::TestSuite 
{
   const Uint K;
   const Uint M;
   vector<uMatrix> vH;
   uVector p_original;
   uVector dir_original;
   vector< vector<LinearMetric> > translationScore;
public:
   TestLinemax()
   : K(2)
   , M(2)
   , p_original(M)
   , dir_original(M)
   {
      /*
      H[0] = [ 1  0 ]
             [ 0  1 ]
      H[1] = [ 1  0 ]
             [ 3 -1 ]
      */
      vH.push_back(uMatrix(K, M));
      vH.back()(0,0) = 1;
      vH.back()(0,1) = 0;
      vH.back()(1,0) = 0;
      vH.back()(1,1) = 1;
      vH.push_back(uMatrix(K, M));
      vH.back()(0,0) = 1;
      vH.back()(0,1) = 0;
      vH.back()(1,0) = 3;
      vH.back()(1,1) = -1;

      // p = (1, 0)^T
      p_original(0) = 1;
      p_original(1) = 0;

      //dir = (0, 1)^T
      dir_original(0) = 0;
      dir_original(1) = 1;

      translationScore.push_back(vector<LinearMetric>());
      translationScore.back().push_back(1);
      translationScore.back().push_back(2);
      translationScore.push_back(vector<LinearMetric>());
      translationScore.back().push_back(1);
      translationScore.back().push_back(2);
   }

   void test_linemax_for_standard_linear_model() {
      uVector p(p_original);
      uVector dir(dir_original);
      LineMax<LinearMetric> linemax(vH, translationScore);
      linemax(p, dir);

      TS_ASSERT_EQUALS(p(0), 1);
      TS_ASSERT_EQUALS(p(1), 1.5);
      TS_ASSERT_EQUALS(evalWeights(p, vH, translationScore).score(), 4);
   }

}; // TestLinemax

} // Portage
