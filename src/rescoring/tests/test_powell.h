/**
 * @author Samuel Larkin
 * @file test_powell.h  Test suite for powell
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2008, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2008, Her Majesty in Right of Canada
 */

#include <cxxtest/TestSuite.h>
#include "portage_defs.h"
#include "powell.h"

using namespace Portage;

namespace Portage {

/// Quick test class for Powell's algorithm
class TestScore
{
   public:
      int  val;
      /// Default constructor.
      TestScore(int val = 0) : val(val) {}
      /// Get score value
      /// @return Returns the score value.
      double score() const { return val; }
      /// Prints score's value for debug mode
      void debugoutput() const { RSC_DEBUG(val); }
      /// Prints score value to out
      /// @param out  output stream to print score's value
      void output(ostream &out) { out << val << endl; }
      /**
       * Substraction operator for TestScore
       * @param other  right-hand side operand
       * @return Returns s1 - s2
       */
      TestScore& operator-=(const TestScore &other)
      {
         val -= other.val;
         return *this;
      }

      /**
       * Addition operator for TestScore
       * @param other  right-hand side operand
       * @return Returns s1 + s2
       */
      TestScore operator+=(const TestScore &other)
      {
         val += other.val;
         return *this;
      }
};

/**
 * Substraction operator for TestScore
 * @param s1  left-hand side operand
 * @param s2  right-hand side operand
 * @return Returns s1 - s2
 */
TestScore operator-(const TestScore &s1, const TestScore &s2)
{
   return TestScore(s1.val - s2.val);
}

/**
 * Addition operator for TestScore
 * @param s1  left-hand side operand
 * @param s2  right-hand side operand
 * @return Returns s1 + s2
 */
TestScore operator+(const TestScore &s1, const TestScore &s2)
{
   return TestScore(s1.val + s2.val);
}


class TestPowell : public CxxTest::TestSuite 
{
   const Uint K;
   const Uint M;
   vector<uMatrix> vH;
   uVector p_original;
   vector< vector<TestScore> > translation_scores;
   vector<string> ft_args;
   int iter;
   double score;
public:
   TestPowell()
   : K(3)
   , M(3)
   , p_original(ublas::scalar_vector<float>(M, 1))
   , iter(0)
   , score(0.0f)
   {
      /*
        H[0] = 
        [ 1  -1   0 ]
        [ 3   1   0 ]
        [ 3   0   1 ]
        H[1] = 
        [ 1  -1   0 ]
        [ 5   1   0 ]
        [ 0   0  -2 ]
      */
      vH.push_back(uMatrix(K, M));
      vH[0](0,0) = 1;
      vH[0](0,1) = -1;
      vH[0](0,2) = 0;
      vH[0](1,0) = 3;
      vH[0](1,1) = 1;
      vH[0](1,2) = 0;
      vH[0](2,0) = 3;
      vH[0](2,1) = 0;
      vH[0](2,2) = 1;
      vH.push_back(uMatrix(K, M));
      vH[1](0,0) = 1;
      vH[1](0,1) = -1;
      vH[1](0,2) = 0;
      vH[1](1,0) = 5;
      vH[1](1,1) = 1;
      vH[1](1,2) = 0;
      vH[1](2,0) = 0;
      vH[1](2,1) = 0;
      vH[1](2,2) = -2;

      /*
        p = (1, 1, 1)^T
      */

      translation_scores.push_back(vector<TestScore>());
      translation_scores.back().push_back(1);
      translation_scores.back().push_back(2);
      translation_scores.back().push_back(3);
      translation_scores.push_back(vector<TestScore>());
      translation_scores.back().push_back(1);
      translation_scores.back().push_back(3);
      translation_scores.back().push_back(2);

      ft_args.push_back("Additive1");
   }

   void test_powell_och_linemax() {
      Powell<TestScore> powell(vH, translation_scores);

      uVector p(p_original);
      powell(p, 0.01, iter, score);

      TS_ASSERT_EQUALS(score, 6);
      TS_ASSERT_EQUALS(iter, 2);
      TS_ASSERT_EQUALS(p(0), 1);
      TS_ASSERT_EQUALS(p(1), -0.5);
      TS_ASSERT_EQUALS(p(2), 0.5);
   }

}; // TestPowell

} // Portage
