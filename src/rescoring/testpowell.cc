/**
 * @author Aaron Tikuisis
 * @file testpowell.cc  Program to test Powell's algorithm.
 * $Id$
 *
 * K-Best Rescoring Module
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2005, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2005, Her Majesty in Right of Canada
 */

#include "powell.h"
#include "rescoring_general.h"
#include "boostDef.h"
#include <vector>


using namespace Portage;
using namespace std;

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


int main()
{
   typedef unsigned int Uint;
   //const Uint S(2);
   const Uint K(5);
   const Uint M(3);

   /*
   H[0] = [ 1  -1   0 ]
          [ 3   1   0 ]
          [ 3   0   1 ]
          [ 4   1  -2 ]
          [-2   -1 -2 ]
   H[1] = [ 1  -1   0 ]
          [ 5   1   0 ]
          [ 0   0  -2 ]
          [ 0   0   7 ]
          [ 1   3   1 ]
   */

   vector<uMatrix> vH;
   vH.push_back(uMatrix(K, M));
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
   vH[0](3,0) = 4;
   vH[0](3,1) = 1;
   vH[0](3,2) = -2;
   vH[0](4,0) = -2;
   vH[0](4,1) = -1;
   vH[0](4,2) = -2;

   vH[1](0,0) = 1;
   vH[1](0,1) = -1;
   vH[1](0,2) = 0;
   vH[1](1,0) = 5;
   vH[1](1,1) = 1;
   vH[1](1,2) = 0;
   vH[1](2,0) = 0;
   vH[1](2,1) = 0;
   vH[1](2,2) = -2;
   vH[1](3,0) = 0;
   vH[1](3,1) = 0;
   vH[1](3,2) = 7;
   vH[1](4,0) = 1;
   vH[1](4,1) = 3;
   vH[1](4,2) = 1;

   /*
     p = (1, 1, 1)^T
   */
   uVector p(M);
   p(0) = 1;
   p(1) = 1;
   p(2) = 1;

   vector< vector<TestScore> > translation_scores;
   translation_scores.push_back(vector<TestScore>());
   translation_scores.back().push_back(1);
   translation_scores.back().push_back(2);
   translation_scores.back().push_back(3);
   translation_scores.back().push_back(2);
   translation_scores.back().push_back(1);
   translation_scores.push_back(vector<TestScore>());
   translation_scores.back().push_back(1);
   translation_scores.back().push_back(3);
   translation_scores.back().push_back(2);
   translation_scores.back().push_back(1);
   translation_scores.back().push_back(4);

   int iter(0);
   double score(0.0f);

   Powell<TestScore> powell(vH, translation_scores);
   powell(p, 0.01, iter, score);

   cout << "p: " << p << p / ublas::norm_inf(p) << endl;
   cout << "vH[0]: " << vH[0] << endl;
   cout << "vH[1]: " << vH[1] << endl;
   cout << "iter: " << iter << endl;
   cout << "score: " << score << endl;
   cout << "vH[0]*p: " << prod(p, vH[0]) << endl;
   cout << "vH[1]*p: " << prod(p, vH[1]) << endl;
}
